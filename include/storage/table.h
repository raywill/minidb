#pragma once

#include "common/types.h"
#include "common/status.h"
#include "storage/catalog.h"
#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace minidb {

// 列文件头部结构
struct ColumnFileHeader {
    uint32_t magic_number;      // 魔数，用于验证文件格式
    uint32_t version;           // 文件版本
    uint32_t data_type;         // 数据类型
    uint64_t row_count;         // 行数
    uint64_t data_offset;       // 数据开始偏移量
    
    static const uint32_t MAGIC = 0x4D494E49; // "MINI"
    static const uint32_t VERSION = 1;
    
    ColumnFileHeader() 
        : magic_number(MAGIC), version(VERSION), data_type(0), 
          row_count(0), data_offset(sizeof(ColumnFileHeader)) {}
};

// 表存储管理器
class Table {
public:
    Table(const std::string& table_name, const TableSchema& schema, const std::string& data_dir);
    ~Table();
    
    // 初始化表
    Status initialize();
    
    // 数据操作
    Status insert_rows(const std::vector<Row>& rows);
    Status delete_rows(const std::vector<size_t>& row_indices);
    Status scan_all(std::vector<ColumnVector>& columns) const;
    Status scan_columns(const std::vector<std::string>& column_names, std::vector<ColumnVector>& columns) const;
    
    // 表信息
    const TableSchema& get_schema() const { return schema_; }
    size_t get_row_count() const;
    
    // 文件操作
    Status flush_all_columns();
    
private:
    std::string table_name_;
    TableSchema schema_;
    std::string data_directory_;
    mutable std::mutex mutex_;
    
    // 辅助方法
    std::string get_column_file_path(const std::string& column_name) const;
    std::string get_column_file_path(size_t column_index) const;
    
    Status load_column_data(size_t column_index, ColumnVector& column) const;
    Status save_column_data(size_t column_index, const ColumnVector& column);
    
    Status read_column_file_header(const std::string& file_path, ColumnFileHeader& header) const;
    
    Status read_column_data_from_file(const std::string& file_path, DataType data_type, 
                                     size_t row_count, ColumnVector& column) const;
    
    // 数据转换
    void rows_to_columns(const std::vector<Row>& rows, std::vector<ColumnVector>& columns) const;
    void columns_to_rows(const std::vector<ColumnVector>& columns, std::vector<Row>& rows) const;
    
    // 文件锁定
    Status lock_table_for_write();
    Status unlock_table_for_write();
};

// 表管理器
class TableManager {
public:
    explicit TableManager(Catalog* catalog);
    ~TableManager();
    
    // 表操作
    Status open_table(const std::string& table_name, std::shared_ptr<Table>& table);
    Status close_table(const std::string& table_name);
    
    // 获取已打开的表
    std::shared_ptr<Table> get_table(const std::string& table_name);
    
private:
    Catalog* catalog_;
    std::unordered_map<std::string, std::shared_ptr<Table>> open_tables_;
    std::mutex mutex_;
};

} // namespace minidb
