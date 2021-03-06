#include "stdafx.h"

#include "FDBFileDB.h"
#include "FDBFieldManager.h"
#include "ExportFormat.h"
#include "utils.h"
#include <iostream>
#include <iomanip>
#include <ctype.h>
using namespace std;



FDBFileDB::FDBFileDB(const FDBPackage::file_info& s_info, uint8_t* data )
    : FDBFile(s_info,data),
    f_info(s_info)
{
    head = (FDBFieldManager::s_file_header*)data;
    entries = data + sizeof(FDBFieldManager::s_file_header);

    assert(head->unknown==0x00006396);
}

bool FDBFileDB::WriteCSV(const char* filename)
{
	DBExport_CSV out(filename);
	return DoExport(out, NULL);
}

bool FDBFileDB::WriteSQLITE(const char* filename)
{
	boost::filesystem::path filepath(filename);
	std::string table1 = filepath.stem().string();

    DBExport_Sqlite3 out(filename);
	return DoExport(out, table1.c_str());
}

bool FDBFileDB::WriteMySQL(const char* filename)
{
	boost::filesystem::path filepath(filename);
	std::string table1 = filepath.stem().string();

    DBExport_MySQL out(filename);
	return DoExport(out, table1.c_str());
}

bool FDBFileDB::DoExport(DBExport& exporter, const char* table_name)
{
	field_list* fields = g_field_manager.GetFieldDefinition(f_info, data);
    if (fields->size()==0) return true;

	return ExportTable(exporter, table_name, fields);
}

bool FDBFileDB::ExportTable(DBExport& exporter, const char* table_name, field_list* fields)
{
	exporter.TableStart(table_name);
	for (field_list::iterator field=fields->begin();field!=fields->end();++field)
	{
		exporter.TableField(field->name,field->position,field->type,field->size);
	}
	exporter.TableEnd();

    if (head->entry_count>0) {
        exporter.EntryHeader();
	    for (uint32_t idx=0;idx<head->entry_count;++idx)
	    {
            exporter.EntryStart();

            uint8_t* line = entries+idx*head->entry_size;

		    for (field_list::iterator field=fields->begin();field!=fields->end();++field)
		    {
			    exporter.EntryField(field->type,line+field->position);
		    }
		    exporter.EntryEnd();
	    }
        exporter.EntryFooter();
    }

	return true;
}

FDB_DBField* FindField(field_list* fields, const string&name)
{
	for (field_list::iterator field=fields->begin();field!=fields->end();++field)
	{
		if (field->name==name) return &field[0];
	}
	return NULL;
}

bool FDBFileDB_LearnMagic::DoExport(DBExport& exporter, const char* table_name)
{
	field_list* fields = g_field_manager.GetFieldDefinition(f_info, data);
	bool r = ExportTable(exporter, table_name, fields);
	if (!r) return false;


	WriteSubArray(exporter, fields, "spmagic");
	WriteSubArray(exporter, fields, "normalmagic");

	return true;
}


bool FDBFileDB_LearnMagic::WriteSubArray(DBExport& exporter, field_list* fields, std::string name)
{
	FDB_DBField* normalmagicinfo = FindField(fields,name+"info");
	FDB_DBField* normalmagiccount = FindField(fields,name+"count");
	if (normalmagicinfo)
	{
		exporter.TableStart(name.c_str());
		exporter.TableField((*fields)[0].name, (*fields)[0].position, (*fields)[0].type, (*fields)[0].size);
		exporter.TableField("id",4,FDB_DBField::F_DWORD,4);
		exporter.TableField("skill",4,FDB_DBField::F_DWORD,4);
		exporter.TableField("min_level",4,FDB_DBField::F_DWORD,4);
		exporter.TableField("req_flag",4,FDB_DBField::F_DWORD,4);
		exporter.TableField("req_skill",4,FDB_DBField::F_DWORD,4);
		exporter.TableField("u1",4,FDB_DBField::F_DWORD,4);
		exporter.TableField("req_skill_lvl",2,FDB_DBField::F_WORD,2);
		exporter.TableField("u3",2,FDB_DBField::F_WORD,2);
		exporter.TableEnd();

        exporter.EntryHeader();
		for (uint32_t idx=0;idx<head->entry_count;++idx)
		{
			uint8_t* line = entries+idx*head->entry_size;
			uint32_t count = *(uint32_t*)(line+normalmagiccount->position);
			assert(count < normalmagicinfo->size/(6*4));

			for (uint32_t i=0;i<count;++i)
			{
				exporter.EntryStart();
				exporter.EntryField((*fields)[0].type, line+ (*fields)[0].position);
				uint32_t temp = i+1;
				exporter.EntryField(FDB_DBField::F_DWORD,&temp);
				exporter.EntryField(FDB_DBField::F_DWORD,line + normalmagicinfo->position+0*4 + i*6*4);
				exporter.EntryField(FDB_DBField::F_DWORD,line + normalmagicinfo->position+1*4 + i*6*4);
				exporter.EntryField(FDB_DBField::F_DWORD,line + normalmagicinfo->position+2*4 + i*6*4);
				exporter.EntryField(FDB_DBField::F_DWORD,line + normalmagicinfo->position+3*4 + i*6*4);
				exporter.EntryField(FDB_DBField::F_DWORD,line + normalmagicinfo->position+4*4 + i*6*4);
				exporter.EntryField(FDB_DBField::F_WORD,line + normalmagicinfo->position+5*4 + i*6*4);
				exporter.EntryField(FDB_DBField::F_WORD,line + normalmagicinfo->position+5*4+2 + i*6*4);

				exporter.EntryEnd();
			}
		}
        exporter.EntryFooter();
	}

	return true;
}


