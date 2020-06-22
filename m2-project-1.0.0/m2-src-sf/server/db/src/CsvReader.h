#pragma once

#include <string>
#include <vector>

#if _MSC_VER
    #include <hash_map>
#else
    #include <map>
#endif

class cCsvAlias
{
private:
#if _MSC_VER
    typedef stdext::hash_map<std::string, uint32_t> NAME2INDEX_MAP;
    typedef stdext::hash_map<uint32_t, std::string> INDEX2NAME_MAP;
#else
    typedef std::map<std::string, uint32_t> NAME2INDEX_MAP;
    typedef std::map<uint32_t, std::string> INDEX2NAME_MAP;
#endif

    NAME2INDEX_MAP m_Name2Index;  
    INDEX2NAME_MAP m_Index2Name;  


public:
    
    cCsvAlias() {} 

    
    virtual ~cCsvAlias() {}


public:
    
    void AddAlias(const char* name, uint32_t index);

    
    void Destroy();

    
    const char* operator [] (uint32_t index) const;

    
    uint32_t operator [] (const char* name) const;


private:
    
    cCsvAlias(const cCsvAlias&) {}

    
    const cCsvAlias& operator = (const cCsvAlias&) { return *this; }
};

class cCsvRow : public std::vector<std::string>
{
public:
    
    cCsvRow() {}

    
    ~cCsvRow() {}


public:
    
    int32_t AsInt(uint32_t index) const { return atoi(at(index).c_str()); }

    
    double AsDouble(uint32_t index) const { return atof(at(index).c_str()); }

    
    const char* AsString(uint32_t index) const { return at(index).c_str(); }

    
    int32_t AsInt(const char* name, const cCsvAlias& alias) const {
        return atoi( at(alias[name]).c_str() ); 
    }

    
    double AsDouble(const char* name, const cCsvAlias& alias) const {
        return atof( at(alias[name]).c_str() ); 
    }

    
    const char* AsString(const char* name, const cCsvAlias& alias) const { 
        return at(alias[name]).c_str(); 
    }


private:
    
    cCsvRow(const cCsvRow&) {}

    
    const cCsvRow& operator = (const cCsvRow&) { return *this; }
};


class cCsvFile
{
private:
    typedef std::vector<cCsvRow*> ROWS;

    ROWS m_Rows; 


public:
    
    cCsvFile() {}

    
    virtual ~cCsvFile() { Destroy(); }


public:
    
    bool Load(const char* fileName, const char seperator=',', const char quote='"');

    
    bool Save(const char* fileName, bool append=false, char seperator=',', char quote='"') const;

    
    void Destroy();

    
    cCsvRow* operator [] (uint32_t index);

    
    const cCsvRow* operator [] (uint32_t index) const;

    
    uint32_t GetRowCount() const { return m_Rows.size(); }


private:
    
    cCsvFile(const cCsvFile&) {}

    
    const cCsvFile& operator = (const cCsvFile&) { return *this; }
};

class cCsvTable
{
public :
    cCsvFile  m_File;   
private:
    cCsvAlias m_Alias;  
    int32_t       m_CurRow; 


public:
    
    cCsvTable();

    
    virtual ~cCsvTable();


public:
    
    bool Load(const char* fileName, const char seperator=',', const char quote='"');

    
    void AddAlias(const char* name, uint32_t index) { m_Alias.AddAlias(name, index); }

    
    bool Next();

    
    uint32_t ColCount() const;

    
    int32_t AsInt(uint32_t index) const;

    
    double AsDouble(uint32_t index) const;

    
    const char* AsStringByIndex(uint32_t index) const;

    
    int32_t AsInt(const char* name) const { return AsInt(m_Alias[name]); }

    
    double AsDouble(const char* name) const { return AsDouble(m_Alias[name]); }

    
    const char* AsString(const char* name) const { return AsStringByIndex(m_Alias[name]); }

    
    void Destroy();


private:
    
    const cCsvRow* const CurRow() const;

    
    cCsvTable(const cCsvTable&) {}

    
    const cCsvTable& operator = (const cCsvTable&) { return *this; }
};
