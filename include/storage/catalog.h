#pragma once

#include "common/types.h"
#include "common/status.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace minidb {

// 表元数据
struct TableMetadata {
    std::string table_name;
    TableSchema schema;
    std::string data_directory;
    size_t row_count;
    
    TableMetadata() : row_count(0) {}
    TableMetadata(const std::string& name, const TableSchema& s, const std::string& dir)
        : table_name(name), schema(s), data_directory(dir), row_count(0) {}
};

// 数据库目录管理器
class Catalog {
public:
    explicit Catalog(const std::string& data_dir);
    ~Catalog();
    
    // 初始化目录
    Status initialize();
    
    // 表操作
    Status create_table(const std::string& table_name, const TableSchema& schema, bool if_not_exists = false);
    Status drop_table(const std::string& table_name, bool if_exists = false);
    
    // 表查询
    bool table_exists(const std::string& table_name) const;
    Status get_table_metadata(const std::string& table_name, TableMetadata& metadata) const;
    std::vector<std::string> list_tables() const;
    
    // 表更新
    Status update_row_count(const std::string& table_name, size_t new_count);
    
    // 持久化
    Status save_metadata();
    Status load_metadata();
    
    // 获取数据目录
    std::string get_data_directory() const { return data_directory_; }
    std::string get_table_directory(const std::string& table_name) const;
    
private:
    std::string data_directory_;
    std::unordered_map<std::string, std::unique_ptr<TableMetadata>> tables_;
    mutable std::mutex mutex_;
    
    // 辅助方法
    std::string get_metadata_file_path() const;
    std::string get_table_schema_file_path(const std::string& table_name) const;
    
    Status create_directory(const std::string& path);
    Status save_table_schema(const std::string& table_name, const TableSchema& schema);
    Status load_table_schema(const std::string& table_name, TableSchema& schema);
};

} // namespace minidb
