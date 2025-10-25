#include "storage/catalog.h"
#include "log/logger.h"
#include "common/utils.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace minidb {

Catalog::Catalog(const std::string& data_dir) : data_directory_(data_dir) {
}

Catalog::~Catalog() {
    save_metadata();
}

Status Catalog::initialize() {
    LOG_INFO("Catalog", "Initialize", "Initializing catalog with data directory: " + data_directory_);
    
    // 创建数据目录
    Status status = create_directory(data_directory_);
    if (!status.ok()) {
        return status;
    }
    
    // 加载元数据
    status = load_metadata();
    if (!status.ok()) {
        LOG_WARN("Catalog", "Initialize", "Failed to load metadata, starting with empty catalog");
        // 不返回错误，允许从空目录开始
    }
    
    LOG_INFO("Catalog", "Initialize", "Catalog initialized successfully");
    return Status::OK();
}

Status Catalog::create_table(const std::string& table_name, const TableSchema& schema, bool if_not_exists) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    LOG_INFO("Catalog", "CreateTable", "Creating table: " + table_name);
    
    if (tables_.find(table_name) != tables_.end()) {
        if (if_not_exists) {
            LOG_INFO("Catalog", "CreateTable", "Table already exists, skipping: " + table_name);
            return Status::OK();
        }
        return Status::AlreadyExists("Table already exists: " + table_name);
    }
    
    // 创建表目录
    std::string table_dir = get_table_directory(table_name);
    Status status = create_directory(table_dir);
    if (!status.ok()) {
        return status;
    }
    
    // 保存表结构
    status = save_table_schema(table_name, schema);
    if (!status.ok()) {
        return status;
    }
    
    // 添加到内存中的元数据
    auto metadata = make_unique<TableMetadata>(table_name, schema, table_dir);
    tables_[table_name] = std::move(metadata);
    
    // 持久化元数据
    status = save_metadata();
    if (!status.ok()) {
        LOG_ERROR("Catalog", "CreateTable", "Failed to save metadata for table: " + table_name);
        return status;
    }
    
    LOG_INFO("Catalog", "CreateTable", "Table created successfully: " + table_name);
    return Status::OK();
}

Status Catalog::drop_table(const std::string& table_name, bool if_exists) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    LOG_INFO("Catalog", "DropTable", "Dropping table: " + table_name);
    
    auto it = tables_.find(table_name);
    if (it == tables_.end()) {
        if (if_exists) {
            LOG_INFO("Catalog", "DropTable", "Table does not exist, skipping: " + table_name);
            return Status::OK();
        }
        return Status::NotFound("Table not found: " + table_name);
    }
    
    // 删除表目录和文件
    std::string table_dir = get_table_directory(table_name);
    std::string rm_command = "rm -rf " + table_dir;
    int result = system(rm_command.c_str());
    if (result != 0) {
        LOG_ERROR("Catalog", "DropTable", "Failed to remove table directory: " + table_dir);
        return Status::IOError("Failed to remove table directory");
    }
    
    // 从内存中移除
    tables_.erase(it);
    
    // 持久化元数据
    Status status = save_metadata();
    if (!status.ok()) {
        LOG_ERROR("Catalog", "DropTable", "Failed to save metadata after dropping table: " + table_name);
        return status;
    }
    
    LOG_INFO("Catalog", "DropTable", "Table dropped successfully: " + table_name);
    return Status::OK();
}

bool Catalog::table_exists(const std::string& table_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tables_.find(table_name) != tables_.end();
}

Status Catalog::get_table_metadata(const std::string& table_name, TableMetadata& metadata) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = tables_.find(table_name);
    if (it == tables_.end()) {
        return Status::NotFound("Table not found: " + table_name);
    }
    
    if (!it->second) {
        LOG_ERROR("Catalog", "GetTableMetadata", "Table metadata pointer is null for table: " + table_name);
        return Status::InternalError("Table metadata is corrupted");
    }
    
    LOG_DEBUG("Catalog", "GetTableMetadata", "Found table metadata for: " + table_name);
    LOG_DEBUG("Catalog", "GetTableMetadata", "Metadata table_name: " + it->second->table_name);
    LOG_DEBUG("Catalog", "GetTableMetadata", "Metadata data_directory: " + it->second->data_directory);
    LOG_DEBUG("Catalog", "GetTableMetadata", "Schema columns count: " + std::to_string(it->second->schema.column_names.size()));
    
    try {
        metadata = *it->second;
        LOG_DEBUG("Catalog", "GetTableMetadata", "Metadata copied successfully");
    } catch (const std::exception& e) {
        LOG_ERROR("Catalog", "GetTableMetadata", "Exception during metadata copy: " + std::string(e.what()));
        return Status::InternalError("Failed to copy table metadata");
    }
    return Status::OK();
}

std::vector<std::string> Catalog::list_tables() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> table_names;
    for (const auto& pair : tables_) {
        table_names.push_back(pair.first);
    }
    return table_names;
}

Status Catalog::update_row_count(const std::string& table_name, size_t new_count) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = tables_.find(table_name);
    if (it == tables_.end()) {
        return Status::NotFound("Table not found: " + table_name);
    }
    
    it->second->row_count = new_count;
    return save_metadata();
}

Status Catalog::save_metadata() {
    std::string metadata_file = get_metadata_file_path();
    std::ofstream file(metadata_file);
    
    if (!file.is_open()) {
        return Status::IOError("Failed to open metadata file for writing: " + metadata_file);
    }
    
    // 简单的JSON格式
    file << "{\n";
    file << "  \"tables\": [\n";
    
    bool first = true;
    for (const auto& pair : tables_) {
        if (!first) {
            file << ",\n";
        }
        first = false;
        
        const TableMetadata& metadata = *pair.second;
        file << "    {\n";
        file << "      \"name\": \"" << metadata.table_name << "\",\n";
        file << "      \"directory\": \"" << metadata.data_directory << "\",\n";
        file << "      \"row_count\": " << metadata.row_count << "\n";
        file << "    }";
    }
    
    file << "\n  ]\n";
    file << "}\n";
    
    file.close();
    return Status::OK();
}

Status Catalog::load_metadata() {
    std::string metadata_file = get_metadata_file_path();
    std::ifstream file(metadata_file);
    
    if (!file.is_open()) {
        // 文件不存在，这是正常的（第一次启动）
        return Status::OK();
    }
    
    // 简单解析（在实际项目中应该使用JSON库）
    std::string line;
    while (std::getline(file, line)) {
        // 跳过JSON格式解析，直接扫描表目录
        break;
    }
    file.close();
    
    // 扫描数据目录中的表
    std::string ls_command = "ls -d " + data_directory_ + "/*/ 2>/dev/null || true";
    FILE* pipe = popen(ls_command.c_str(), "r");
    if (!pipe) {
        return Status::IOError("Failed to list table directories");
    }
    
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string dir_path(buffer);
        // 移除换行符
        if (!dir_path.empty() && dir_path.back() == '\n') {
            dir_path.pop_back();
        }
        // 移除末尾的斜杠
        if (!dir_path.empty() && dir_path.back() == '/') {
            dir_path.pop_back();
        }
        
        // 提取表名
        size_t last_slash = dir_path.find_last_of('/');
        if (last_slash != std::string::npos) {
            std::string table_name = dir_path.substr(last_slash + 1);
            
            // 加载表结构
            TableSchema schema;
            Status status = load_table_schema(table_name, schema);
            if (status.ok()) {
                auto metadata = make_unique<TableMetadata>(table_name, schema, dir_path);
                tables_[table_name] = std::move(metadata);
                LOG_INFO("Catalog", "LoadMetadata", "Loaded table: " + table_name);
            }
        }
    }
    
    pclose(pipe);
    return Status::OK();
}

std::string Catalog::get_table_directory(const std::string& table_name) const {
    return data_directory_ + "/" + table_name;
}

std::string Catalog::get_metadata_file_path() const {
    return data_directory_ + "/db.meta.json";
}

std::string Catalog::get_table_schema_file_path(const std::string& table_name) const {
    return get_table_directory(table_name) + "/schema.json";
}

Status Catalog::create_directory(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return Status::OK(); // 目录已存在
        } else {
            return Status::IOError("Path exists but is not a directory: " + path);
        }
    }
    
    // 创建目录
    if (mkdir(path.c_str(), 0755) != 0) {
        return Status::IOError("Failed to create directory: " + path);
    }
    
    return Status::OK();
}

Status Catalog::save_table_schema(const std::string& table_name, const TableSchema& schema) {
    std::string schema_file = get_table_schema_file_path(table_name);
    std::ofstream file(schema_file);
    
    if (!file.is_open()) {
        return Status::IOError("Failed to open schema file for writing: " + schema_file);
    }
    
    // 简单的JSON格式
    file << "{\n";
    file << "  \"table_name\": \"" << schema.table_name << "\",\n";
    file << "  \"columns\": [\n";
    
    for (size_t i = 0; i < schema.column_names.size(); ++i) {
        if (i > 0) {
            file << ",\n";
        }
        file << "    {\n";
        file << "      \"name\": \"" << schema.column_names[i] << "\",\n";
        file << "      \"type\": \"" << DataTypeToString(schema.column_types[i]) << "\"\n";
        file << "    }";
    }
    
    file << "\n  ]\n";
    file << "}\n";
    
    file.close();
    return Status::OK();
}

Status Catalog::load_table_schema(const std::string& table_name, TableSchema& schema) {
    std::string schema_file = get_table_schema_file_path(table_name);
    std::ifstream file(schema_file);
    
    if (!file.is_open()) {
        return Status::IOError("Failed to open schema file for reading: " + schema_file);
    }
    
    schema.table_name = table_name;
    schema.column_names.clear();
    schema.column_types.clear();
    
    // 简单解析（在实际项目中应该使用JSON库）
    std::string line;
    bool in_columns = false;
    std::string current_name;
    std::string current_type;
    
    while (std::getline(file, line)) {
        // 移除空白字符
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (line.find("\"columns\"") != std::string::npos) {
            in_columns = true;
            continue;
        }
        
        if (in_columns) {
            if (line.find("\"name\"") != std::string::npos) {
                size_t start = line.find(":") + 1;
                size_t first_quote = line.find("\"", start);
                size_t second_quote = line.find("\"", first_quote + 1);
                if (first_quote != std::string::npos && second_quote != std::string::npos) {
                    current_name = line.substr(first_quote + 1, second_quote - first_quote - 1);
                }
            } else if (line.find("\"type\"") != std::string::npos) {
                size_t start = line.find(":") + 1;
                size_t first_quote = line.find("\"", start);
                size_t second_quote = line.find("\"", first_quote + 1);
                if (first_quote != std::string::npos && second_quote != std::string::npos) {
                    current_type = line.substr(first_quote + 1, second_quote - first_quote - 1);
                    
                    // 添加列
                    if (!current_name.empty() && !current_type.empty()) {
                        schema.column_names.push_back(current_name);
                        schema.column_types.push_back(StringToDataType(current_type));
                        current_name.clear();
                        current_type.clear();
                    }
                }
            }
        }
    }
    
    file.close();
    return Status::OK();
}

} // namespace minidb
