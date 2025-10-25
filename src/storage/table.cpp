#include "storage/table.h"
#include "log/logger.h"
#include "common/utils.h"
#include <fstream>
#include <algorithm>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace minidb {

Table::Table(const std::string& table_name, const TableSchema& schema, const std::string& data_dir)
    : table_name_(table_name), schema_(schema), data_directory_(data_dir) {
    LOG_DEBUG("Table", table_name_, "Table constructor called with data_dir: " + data_dir);
    LOG_DEBUG("Table", table_name_, "Schema has " + std::to_string(schema.column_names.size()) + " columns");
}

Table::~Table() {
    flush_all_columns();
}

Status Table::initialize() {
    LOG_INFO("Table", table_name_, "Initializing table");
    
    // 检查数据目录是否存在
    struct stat st;
    if (stat(data_directory_.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
        return Status::IOError("Table data directory does not exist: " + data_directory_);
    }
    
    LOG_INFO("Table", table_name_, "Table initialized successfully");
    return Status::OK();
}

Status Table::insert_rows(const std::vector<Row>& rows) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    LOG_INFO("Table", table_name_, "Inserting " + std::to_string(rows.size()) + " rows");
    
    if (rows.empty()) {
        return Status::OK();
    }
    
    // 验证行数据
    for (const auto& row : rows) {
        if (row.values.size() != schema_.column_names.size()) {
            return Status::InvalidArgument("Row has incorrect number of columns");
        }
    }
    
    // 加载现有数据（不使用scan_all避免死锁）
    LOG_DEBUG("Table", table_name_, "Loading existing data");
    std::vector<ColumnVector> existing_columns;
    existing_columns.resize(schema_.column_names.size());
    
    bool table_is_empty = false;
    for (size_t i = 0; i < schema_.column_names.size(); ++i) {
        Status status = load_column_data(i, existing_columns[i]);
        if (!status.ok()) {
            if (status.is_not_found()) {
                // 列文件不存在，表示表为空
                table_is_empty = true;
                break;
            }
            LOG_ERROR("Table", table_name_, "Failed to load existing data: " + status.ToString());
            return status;
        }
    }
    
    // 如果表为空，初始化列向量
    if (table_is_empty) {
        LOG_DEBUG("Table", table_name_, "Table is empty, initializing columns");
        existing_columns.clear();
        existing_columns.resize(schema_.column_names.size());
        for (size_t i = 0; i < schema_.column_names.size(); ++i) {
            existing_columns[i].name = schema_.column_names[i];
            existing_columns[i].type = schema_.column_types[i];
        }
    }
    
    LOG_DEBUG("Table", table_name_, "Existing columns loaded, count: " + std::to_string(existing_columns.size()));
    
    // 将新行转换为列格式并追加
    LOG_DEBUG("Table", table_name_, "Converting rows to columns");
    std::vector<ColumnVector> new_columns;
    rows_to_columns(rows, new_columns);
    LOG_DEBUG("Table", table_name_, "Rows converted to columns");
    
    for (size_t i = 0; i < existing_columns.size(); ++i) {
        // 追加新数据
        const auto& new_col = new_columns[i];
        auto& existing_col = existing_columns[i];
        
        existing_col.data.insert(existing_col.data.end(), 
                               new_col.data.begin(), new_col.data.end());
        existing_col.size += new_col.size;
    }
    
    // 保存所有列
    LOG_DEBUG("Table", table_name_, "Saving " + std::to_string(existing_columns.size()) + " columns");
    for (size_t i = 0; i < existing_columns.size(); ++i) {
        LOG_DEBUG("Table", table_name_, "Saving column " + std::to_string(i) + " (" + existing_columns[i].name + ")");
        Status save_status = save_column_data(i, existing_columns[i]);
        if (!save_status.ok()) {
            LOG_ERROR("Table", table_name_, "Failed to save column " + std::to_string(i) + ": " + save_status.ToString());
            return save_status;
        }
        LOG_DEBUG("Table", table_name_, "Column " + std::to_string(i) + " saved successfully");
    }
    
    LOG_INFO("Table", table_name_, "Rows inserted successfully");
    return Status::OK();
}

Status Table::delete_rows(const std::vector<size_t>& row_indices) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    LOG_INFO("Table", table_name_, "Deleting " + std::to_string(row_indices.size()) + " rows");
    
    if (row_indices.empty()) {
        return Status::OK();
    }
    
    // 加载所有列数据
    std::vector<ColumnVector> columns;
    Status status = scan_all(columns);
    if (!status.ok()) {
        return status;
    }
    
    if (columns.empty()) {
        return Status::OK(); // 表为空
    }
    
    size_t total_rows = columns[0].size;
    
    // 验证行索引
    for (size_t idx : row_indices) {
        if (idx >= total_rows) {
            return Status::InvalidArgument("Row index out of range: " + std::to_string(idx));
        }
    }
    
    // 创建删除标记
    std::vector<bool> to_delete(total_rows, false);
    for (size_t idx : row_indices) {
        to_delete[idx] = true;
    }
    
    // 重建每一列的数据
    for (auto& column : columns) {
        std::vector<uint8_t> new_data;
        size_t new_size = 0;
        
        if (column.type == DataType::STRING) {
            // 字符串类型需要特殊处理
            size_t offset = 0;
            for (size_t i = 0; i < total_rows; ++i) {
                if (!to_delete[i]) {
                    // 读取字符串长度
                    uint32_t length = *reinterpret_cast<const uint32_t*>(&column.data[offset]);
                    // 复制长度和字符串数据
                    const uint8_t* src = &column.data[offset];
                    new_data.insert(new_data.end(), src, src + sizeof(uint32_t) + length);
                    new_size++;
                }
                
                // 移动到下一个字符串
                uint32_t length = *reinterpret_cast<const uint32_t*>(&column.data[offset]);
                offset += sizeof(uint32_t) + length;
            }
        } else {
            // 固定大小类型
            size_t type_size = GetDataTypeSize(column.type);
            for (size_t i = 0; i < total_rows; ++i) {
                if (!to_delete[i]) {
                    const uint8_t* src = &column.data[i * type_size];
                    new_data.insert(new_data.end(), src, src + type_size);
                    new_size++;
                }
            }
        }
        
        column.data = std::move(new_data);
        column.size = new_size;
    }
    
    // 保存所有列
    for (size_t i = 0; i < columns.size(); ++i) {
        status = save_column_data(i, columns[i]);
        if (!status.ok()) {
            LOG_ERROR("Table", table_name_, "Failed to save column " + std::to_string(i));
            return status;
        }
    }
    
    LOG_INFO("Table", table_name_, "Rows deleted successfully");
    return Status::OK();
}

Status Table::scan_all(std::vector<ColumnVector>& columns) const {
    std::lock_guard<std::mutex> lock(mutex_);

    LOG_DEBUG("Table", table_name_, "Starting scan_all");
    columns.clear();
    columns.resize(schema_.column_names.size());

    for (size_t i = 0; i < schema_.column_names.size(); ++i) {
        LOG_DEBUG("Table", table_name_, "Loading column " + std::to_string(i) + " (" + schema_.column_names[i] + ")");
        Status status = load_column_data(i, columns[i]);
        if (!status.ok()) {
            if (status.is_not_found()) {
                // 列文件不存在，表示表为空 - 这是正常情况
                LOG_DEBUG("Table", table_name_, "Column file not found, table is empty");
                columns.clear();
                // 初始化空的列向量
                columns.resize(schema_.column_names.size());
                for (size_t j = 0; j < schema_.column_names.size(); ++j) {
                    columns[j].name = schema_.column_names[j];
                    columns[j].type = schema_.column_types[j];
                    columns[j].size = 0;
                }
                return Status::OK(); // 空表返回OK，不是错误
            }
            LOG_ERROR("Table", table_name_, "Failed to load column " + std::to_string(i) + ": " + status.ToString());
            return status;
        }
        LOG_DEBUG("Table", table_name_, "Column " + std::to_string(i) + " loaded successfully");
    }

    LOG_DEBUG("Table", table_name_, "scan_all completed successfully");
    return Status::OK();
}

Status Table::scan_columns(const std::vector<std::string>& column_names, std::vector<ColumnVector>& columns) const {
    std::lock_guard<std::mutex> lock(mutex_);

    columns.clear();
    columns.resize(column_names.size());

    for (size_t i = 0; i < column_names.size(); ++i) {
        int column_index = schema_.get_column_index(column_names[i]);
        if (column_index < 0) {
            return Status::NotFound("Column not found: " + column_names[i]);
        }

        Status status = load_column_data(column_index, columns[i]);
        if (!status.ok()) {
            if (status.is_not_found()) {
                // 列文件不存在，表示表为空 - 这是正常情况
                columns.clear();
                // 初始化空的列向量
                columns.resize(column_names.size());
                for (size_t j = 0; j < column_names.size(); ++j) {
                    int idx = schema_.get_column_index(column_names[j]);
                    columns[j].name = column_names[j];
                    columns[j].type = schema_.column_types[idx];
                    columns[j].size = 0;
                }
                return Status::OK(); // 空表返回OK，不是错误
            }
            return status;
        }
    }

    return Status::OK();
}

size_t Table::get_row_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (schema_.column_names.empty()) {
        return 0;
    }
    
    // 读取第一列的行数
    ColumnVector first_column;
    Status status = load_column_data(0, first_column);
    if (!status.ok()) {
        return 0;
    }
    
    return first_column.size;
}

Status Table::flush_all_columns() {
    // 在当前实现中，数据已经立即写入磁盘
    // 这里可以添加缓存刷新逻辑
    return Status::OK();
}

std::string Table::get_column_file_path(const std::string& column_name) const {
    return data_directory_ + "/" + column_name + ".col";
}

std::string Table::get_column_file_path(size_t column_index) const {
    return data_directory_ + "/col_" + std::to_string(column_index) + ".bin";
}

Status Table::load_column_data(size_t column_index, ColumnVector& column) const {
    if (column_index >= schema_.column_names.size()) {
        return Status::InvalidArgument("Column index out of range");
    }
    
    std::string file_path = get_column_file_path(column_index);
    LOG_DEBUG("Table", table_name_, "Loading column data from file: " + file_path);
    
    // 读取文件头
    ColumnFileHeader header;
    Status status = read_column_file_header(file_path, header);
    if (!status.ok()) {
        LOG_DEBUG("Table", table_name_, "Failed to read column file header: " + status.ToString());
        return status;
    }
    
    // 初始化列向量
    column.name = schema_.column_names[column_index];
    column.type = schema_.column_types[column_index];
    column.size = header.row_count;
    
    LOG_DEBUG("Table", table_name_, "Column header loaded, row_count: " + std::to_string(header.row_count));
    
    // 读取数据
    status = read_column_data_from_file(file_path, column.type, header.row_count, column);
    if (!status.ok()) {
        LOG_DEBUG("Table", table_name_, "Failed to read column data: " + status.ToString());
        return status;
    }
    
    LOG_DEBUG("Table", table_name_, "Column data loaded successfully");
    return Status::OK();
}

Status Table::save_column_data(size_t column_index, const ColumnVector& column) {
    if (column_index >= schema_.column_names.size()) {
        return Status::InvalidArgument("Column index out of range");
    }
    
    std::string file_path = get_column_file_path(column_index);
    
    // 创建文件头
    ColumnFileHeader header;
    header.data_type = static_cast<uint32_t>(column.type);
    header.row_count = column.size;
    
    // 一次性写入整个文件（文件头 + 数据）
    std::ofstream file(file_path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        return Status::IOError("Failed to open column file for writing: " + file_path);
    }
    
    // 写入文件头
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    if (!file.good()) {
        return Status::IOError("Failed to write column file header: " + file_path);
    }
    
    // 写入数据
    file.write(reinterpret_cast<const char*>(column.data.data()), column.data.size());
    if (!file.good()) {
        return Status::IOError("Failed to write column data: " + file_path);
    }
    
    file.close();
    
    // 确保数据写入磁盘
    sync();
    
    return Status::OK();
}

Status Table::read_column_file_header(const std::string& file_path, ColumnFileHeader& header) const {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return Status::NotFound("Column file not found: " + file_path);
    }
    
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (file.gcount() != sizeof(header)) {
        return Status::IOError("Failed to read column file header: " + file_path);
    }
    
    // 验证魔数
    if (header.magic_number != ColumnFileHeader::MAGIC) {
        return Status::IOError("Invalid column file format: " + file_path);
    }
    
    return Status::OK();
}


Status Table::read_column_data_from_file(const std::string& file_path, DataType data_type, 
                                        size_t row_count, ColumnVector& column) const {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return Status::IOError("Failed to open column file for reading: " + file_path);
    }
    
    // 跳过文件头
    file.seekg(sizeof(ColumnFileHeader));
    
    column.data.clear();
    
    if (data_type == DataType::STRING) {
        // 字符串类型：读取所有数据
        file.seekg(0, std::ios::end);
        size_t file_size = file.tellg();
        file.seekg(sizeof(ColumnFileHeader));
        
        size_t data_size = file_size - sizeof(ColumnFileHeader);
        column.data.resize(data_size);
        
        file.read(reinterpret_cast<char*>(column.data.data()), data_size);
        if (file.gcount() != static_cast<std::streamsize>(data_size)) {
            return Status::IOError("Failed to read string column data: " + file_path);
        }
    } else {
        // 固定大小类型
        size_t type_size = GetDataTypeSize(data_type);
        size_t total_size = row_count * type_size;
        
        column.data.resize(total_size);
        file.read(reinterpret_cast<char*>(column.data.data()), total_size);
        if (file.gcount() != static_cast<std::streamsize>(total_size)) {
            return Status::IOError("Failed to read column data: " + file_path);
        }
    }
    
    return Status::OK();
}


void Table::rows_to_columns(const std::vector<Row>& rows, std::vector<ColumnVector>& columns) const {
    columns.clear();
    columns.resize(schema_.column_names.size());
    
    // 初始化列向量
    for (size_t i = 0; i < schema_.column_names.size(); ++i) {
        columns[i].name = schema_.column_names[i];
        columns[i].type = schema_.column_types[i];
        columns[i].size = 0;  // 初始化为0，append方法会自动增加
        columns[i].reserve(rows.size());
    }
    
    // 转换数据
    for (const auto& row : rows) {
        for (size_t i = 0; i < row.values.size() && i < columns.size(); ++i) {
            const std::string& value = row.values[i];
            
            switch (columns[i].type) {
                case DataType::INT: {
                    int32_t int_val = std::stoi(value);
                    columns[i].append_int(int_val);
                    break;
                }
                case DataType::STRING:
                    columns[i].append_string(value);
                    break;
                case DataType::BOOL: {
                    bool bool_val = (value == "true" || value == "TRUE" || value == "1");
                    columns[i].append_bool(bool_val);
                    break;
                }
                case DataType::DECIMAL: {
                    double decimal_val = std::stod(value);
                    columns[i].append_decimal(decimal_val);
                    break;
                }
            }
        }
    }
}

void Table::columns_to_rows(const std::vector<ColumnVector>& columns, std::vector<Row>& rows) const {
    if (columns.empty()) {
        rows.clear();
        return;
    }
    
    size_t row_count = columns[0].size;
    rows.resize(row_count);
    
    for (size_t i = 0; i < row_count; ++i) {
        rows[i].values.resize(columns.size());
        
        for (size_t j = 0; j < columns.size(); ++j) {
            switch (columns[j].type) {
                case DataType::INT:
                    rows[i].values[j] = std::to_string(columns[j].get_int(i));
                    break;
                case DataType::STRING:
                    rows[i].values[j] = columns[j].get_string(i);
                    break;
                case DataType::BOOL:
                    rows[i].values[j] = columns[j].get_bool(i) ? "true" : "false";
                    break;
                case DataType::DECIMAL:
                    rows[i].values[j] = std::to_string(columns[j].get_decimal(i));
                    break;
            }
        }
    }
}

Status Table::lock_table_for_write() {
    // 简单实现：使用文件锁
    // 在实际项目中可能需要更复杂的锁机制
    return Status::OK();
}

Status Table::unlock_table_for_write() {
    return Status::OK();
}

// TableManager 实现
TableManager::TableManager(Catalog* catalog) : catalog_(catalog) {
}

TableManager::~TableManager() {
    // 关闭所有打开的表
    std::lock_guard<std::mutex> lock(mutex_);
    open_tables_.clear();
}

Status TableManager::open_table(const std::string& table_name, std::shared_ptr<Table>& table) {
    LOG_DEBUG("TableManager", "OpenTable", "Attempting to open table: " + table_name);
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 检查表是否已经打开
    auto it = open_tables_.find(table_name);
    if (it != open_tables_.end()) {
        LOG_DEBUG("TableManager", "OpenTable", "Table already open, returning cached instance");
        table = it->second;
        return Status::OK();
    }
    
    LOG_DEBUG("TableManager", "OpenTable", "Table not cached, loading from catalog");
    
    // 从目录中获取表元数据
    TableMetadata metadata;
    Status status = catalog_->get_table_metadata(table_name, metadata);
    if (!status.ok()) {
        LOG_ERROR("TableManager", "OpenTable", "Failed to get table metadata: " + status.ToString());
        return status;
    }
    
    LOG_DEBUG("TableManager", "OpenTable", "Table metadata - name: " + metadata.table_name + 
              ", directory: " + metadata.data_directory + ", columns: " + std::to_string(metadata.schema.column_names.size()));
    
    // 验证元数据
    if (metadata.data_directory.empty()) {
        LOG_ERROR("TableManager", "OpenTable", "Empty data directory for table: " + table_name);
        return Status::InternalError("Invalid table metadata: empty data directory");
    }
    
    if (metadata.schema.column_names.empty()) {
        LOG_ERROR("TableManager", "OpenTable", "Empty schema for table: " + table_name);
        return Status::InternalError("Invalid table metadata: empty schema");
    }
    
    // 创建表对象
    LOG_DEBUG("TableManager", "OpenTable", "Creating Table object");
    Table* raw_table = nullptr;
    try {
        raw_table = new Table(table_name, metadata.schema, metadata.data_directory);
        LOG_DEBUG("TableManager", "OpenTable", "Table object created successfully");
    } catch (const std::exception& e) {
        LOG_ERROR("TableManager", "OpenTable", "Exception creating Table object: " + std::string(e.what()));
        return Status::InternalError("Failed to create table object");
    }
    
    auto new_table = std::shared_ptr<Table>(raw_table);
    LOG_DEBUG("TableManager", "OpenTable", "Table wrapped in shared_ptr");
    
    status = new_table->initialize();
    if (!status.ok()) {
        return status;
    }
    
    // 缓存表对象
    open_tables_[table_name] = new_table;
    table = new_table;
    
    LOG_INFO("TableManager", "OpenTable", "Opened table: " + table_name);
    return Status::OK();
}

Status TableManager::close_table(const std::string& table_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = open_tables_.find(table_name);
    if (it != open_tables_.end()) {
        open_tables_.erase(it);
        LOG_INFO("TableManager", "CloseTable", "Closed table: " + table_name);
    }
    
    return Status::OK();
}

std::shared_ptr<Table> TableManager::get_table(const std::string& table_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = open_tables_.find(table_name);
    if (it != open_tables_.end()) {
        return it->second;
    }
    
    return nullptr;
}

} // namespace minidb
