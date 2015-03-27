/*
 * Copyright 2015 ZHIHUI WEN (tzw.wen@gmail.com)

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#ifndef __CBOOSTREADONLYMAPFILE_H__
#define __CBOOSTREADONLYMAPFILE_H__

#include "StringHelper.h"

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/filesystem/operations.hpp>         // for real file size
//
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>

///////////////////////////////////////////////////////
#include "FILEMAPHELPER_Export.h"
class CBoostReadOnlyMapFile
{
private:
	const static _tstring m_sErrorHead;

public:
	_tstring m_sFilePath;     // path to the current file
    __uint64 m_nPageStart;       // the starting page pointer
    __uint64 m_nPageOffset;     // current file pointer
    __uint64 m_nPageSize;      // a single page size
	__uint64 m_nMax_Num_Pages;     // the number of pages
    __uint64 m_nMemoryMapSize;        // memory map size
    __uint64 m_nFileSize;   // real file size
    __uint64 m_nFileSizeLeft;       // still left in the file

    boost::iostreams::mapped_file_source m_memoryMappedFile; // current memory map source
		
public:
	CBoostReadOnlyMapFile(const _tstring & sFilePath, __uint64 & nMax_Num_Pages);
	
	~CBoostReadOnlyMapFile(void);

public:
	const __uint64 Residual() const;

	const std::streamsize Lines(const TCHAR & cEOL) const;

	const size_t Lines(std::vector<_tstring> & vectorAllLines) const;

	bool IsOpen() const;

	void Close();

	//
	// reads into buffer, the specified num_bytes
	//
	__uint64 Read_bytes (void * buffer, __uint64 & num_bytes);

public:
	static int ReadFileInChar(const _tstring & sSourceFile, std::string & sCharOut, _tstring & sError);

	static int ReadFileInByte(const _tstring & sSourceFile, std::vector<byte> & vectorByteOut, _tstring & sError);

	static std::streamsize CountFileLines(const _tstring & sSourceFile, const TCHAR & cEOL,  _tstring & sError);
	
	static size_t GetFileLines(const _tstring & sSourceFile, std::vector<_tstring> & vectorAllLines, _tstring & sError);

	static int WriteFileCopy(const _tstring & sSourceFile, const _tstring & sTargetFile, const bool & bOverwrite, _tstring & sError);
};
///////////////////////////////////////////////////////
#include "FILEMAPHELPER_Deport.h"


#endif //!__CBOOSTREADONLYMAPFILE_H__
