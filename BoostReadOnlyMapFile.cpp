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

#include "BoostReadOnlyMapFile.h"

const _tstring CBoostReadOnlyMapFile::m_sErrorHead = _T("Error:");

CBoostReadOnlyMapFile::CBoostReadOnlyMapFile(const _tstring & sFilePath, __uint64 & nMax_Num_Pages): 
	m_sFilePath(sFilePath),                                       // file path
	m_nPageStart(0),                                    // starting page
	m_nPageOffset(0),                                  // starting page offset		 
	m_nPageSize(m_memoryMappedFile.alignment()),                 // a single page size  
	m_nFileSize(boost::filesystem::file_size(sFilePath) // real size file
	)
	{	
		m_nMax_Num_Pages = m_nFileSize/m_nPageSize+1;
		nMax_Num_Pages = m_nMax_Num_Pages;
		m_nFileSizeLeft = m_nFileSize;
		m_nMemoryMapSize = m_nPageSize*m_nMax_Num_Pages;                 // file map size still left in file                                        
		m_memoryMappedFile = boost::iostreams::mapped_file_source(
			boost::filesystem::path(sFilePath.c_str()),                                      // path  
			m_nMemoryMapSize > m_nFileSize ? m_nFileSize : m_nMemoryMapSize, // map_size
			0  // initial offset 
			);

		//std::cout << "m_nFileSize/m_nPageSize+1:" << m_nFileSize/m_nPageSize+1 << std::endl;		
		//std::cout << "m_nMax_Num_Pages:" << m_nMax_Num_Pages << std::endl;
		//std::cout << "m_nMemoryMapSize:" << m_nMemoryMapSize << std::endl;
		//std::cout << "Page size: " << m_nPageSize << std::endl; 
		//std::cout << "Paged Size: " << m_memoryMappedFile.size() << std::endl;
		//std::cout << "File Size: " << m_nFileSize << std::endl;
	};

CBoostReadOnlyMapFile::~CBoostReadOnlyMapFile(void)
{
	Close();	
}

bool CBoostReadOnlyMapFile::IsOpen() const{
	return m_memoryMappedFile.is_open();
}

void CBoostReadOnlyMapFile::Close(){
	if(m_memoryMappedFile.is_open()){
		m_memoryMappedFile.close();
	}
}

const __uint64 CBoostReadOnlyMapFile::Residual() const { 
	return m_nFileSizeLeft; 
}

const std::streamsize CBoostReadOnlyMapFile::Lines(const TCHAR & cEOL) const {
	return std::count(m_memoryMappedFile.data(), m_memoryMappedFile.data() + m_memoryMappedFile.size(), cEOL);
}

const size_t CBoostReadOnlyMapFile::Lines(std::vector<_tstring> & vectorAllLines) const {
	boost::iostreams::stream<boost::iostreams::mapped_file_source> memoryMappedFileStream(m_memoryMappedFile); //current memory map source stream
	_tstring sLine;
	vectorAllLines.clear();
	while(std::getline(memoryMappedFileStream, sLine)){
		vectorAllLines.push_back(sLine);
	}
	//
	return vectorAllLines.size();
}
//
// reads into buffer, the specified num_bytes
//   num_bytes must be a multiple of page size
//
__uint64 CBoostReadOnlyMapFile::Read_bytes (void * buffer, __uint64 & num_bytes){

	//assert (num_bytes < m_nMemoryMapSize);// dont read more than a page
	if(num_bytes > m_nMemoryMapSize){// dont read more than a page
		num_bytes = m_nMemoryMapSize;
	}

	//assert ((num_bytes % m_nPageSize)==0); // multiple of the page
	if((num_bytes % m_nPageSize)==0){
		num_bytes = m_nPageSize * (num_bytes / m_nPageSize+1);
	}

	if (num_bytes > m_nFileSizeLeft)    // don't read more than what 
		num_bytes = m_nFileSizeLeft;      // we have available

	const __uint64 end_pointer = m_nPageOffset + num_bytes;
	if (end_pointer >= m_nMemoryMapSize){// repage
		m_nPageStart += m_nPageOffset;// record next page start		
		m_memoryMappedFile.close ();            // close current page
		//the following code acutally crashed. So in CBoostReadOnlyMapFile() 
		//we set nMax_Num_Pages to 
		//max enough that end_pointer >= m_nMemoryMapSize never happened 
		m_memoryMappedFile.open (boost::filesystem::path(m_sFilePath.c_str()), 
			m_nMemoryMapSize, 
			m_nPageStart); // open next page
		m_nPageOffset = 0;           // zero offset
	}
	
	memcpy(buffer, m_memoryMappedFile.data()+m_nPageOffset, num_bytes);
	m_nPageOffset += num_bytes;    // increment this page off_set
	m_nFileSizeLeft -= num_bytes;      // still left in file

	return num_bytes;              // bytes we read
};

int CBoostReadOnlyMapFile::ReadFileInByte(
	const _tstring & sSourceFile, 
	std::vector<byte> & vectorByteOut, 
	_tstring & sError){
	vectorByteOut.clear();
	//
	boost::filesystem::path pathSource = boost::filesystem::path(sSourceFile.c_str());
	//
	if(!boost::filesystem::exists(pathSource)){
		std::stringstream ss;
		ss << m_sErrorHead << _T("source file:") << sSourceFile << _T(" did not exist.");
		sError.append(ss.str());
		//
		return -1;
	}
	//
	if(boost::filesystem::is_directory(pathSource)){
		std::stringstream ss;
		ss << m_sErrorHead << _T("source file:") << sSourceFile << _T(" was a folder.");
		sError.append(ss.str());
		//
		return -2;
	}
	//
	//we could chose to write memory:
	//1. snprintf 
	//2. ostringstream
	//3. boost::iostreams::stream<boost::iostreams::array_sink>
	//4. boost::iostreams::stream<boost::iostreams::back_insert_device<vector> >
	//but speed was: 1:2:3:4 = 1 : 2.5 : 2 : 2.2 
	//
	//
	__uint64 nMax_Num_Pages = 1;
	const int buf_size=1024;
	char buffer[buf_size+1];
	CBoostReadOnlyMapFile sourceMappedFile(sSourceFile,
		nMax_Num_Pages);
	if(!sourceMappedFile.IsOpen()){
		std::stringstream ss;
		ss << m_sErrorHead << _T("could not open a maped file with source file:")
			<< sSourceFile;
		sError.append(ss.str());
		//
		return -10;
	}
	//
	if(vectorByteOut.max_size() < sourceMappedFile.Residual()){
		std::stringstream ss;
		ss << m_sErrorHead << _T("max allowed string size in byte:") << vectorByteOut.max_size()
			<< _T(" was < file size in byte:") << sourceMappedFile.Residual();
		sError.append(ss.str());
		//
		sourceMappedFile.Close();
		//
		return -11;
	}
	//
	vectorByteOut.reserve(sourceMappedFile.Residual());
	//
	unsigned long nReadingCycles = 0L;
	unsigned long nReadingBytes = 0L;
	__uint64 buflen = buf_size*sizeof(buffer[0]);
	while (sourceMappedFile.Residual()>0){		
		__uint64 l = sourceMappedFile.Read_bytes(buffer, buflen);
		buffer[l] = '\0';
		//
		vectorByteOut.resize(vectorByteOut.size()+l);
		//
		std::memcpy(&vectorByteOut[nReadingBytes], buffer, l);
		//
		nReadingBytes += l;
		++nReadingCycles;
	}
	//	
	sourceMappedFile.Close();
	//
	return 1;
}

int CBoostReadOnlyMapFile::ReadFileInChar(
	const _tstring & sSourceFile, 
	std::string & sCharOut, 
	_tstring & sError){
	sCharOut.clear();
	//
	boost::filesystem::path pathSource = boost::filesystem::path(sSourceFile.c_str());
	//
	if(!boost::filesystem::exists(pathSource)){
		std::stringstream ss;
		ss << m_sErrorHead << _T("source file:") << sSourceFile << _T(" did not exist.");
		sError.append(ss.str());
		//
		return -1;
	}
	//
	if(boost::filesystem::is_directory(pathSource)){
		std::stringstream ss;
		ss << m_sErrorHead << _T("source file:") << sSourceFile << _T(" was a folder.");
		sError.append(ss.str());
		//
		return -2;
	}
	//
	//we could chose to write memory:
	//1. snprintf 
	//2. ostringstream
	//3. boost::iostreams::stream<boost::iostreams::array_sink>
	//4. boost::iostreams::stream<boost::iostreams::back_insert_device<vector> >
	//but speed was: 1:2:3:4 = 1 : 2.5 : 2 : 2.2 
	//
	//
	__uint64 nMax_Num_Pages = 1;
	const int buf_size=1024;
	char buffer[buf_size+1];
	CBoostReadOnlyMapFile sourceMappedFile(sSourceFile,
		nMax_Num_Pages);
	if(!sourceMappedFile.IsOpen()){
		std::stringstream ss;
		ss << m_sErrorHead << _T("could not open a maped file with source file:")
			<< sSourceFile;
		sError.append(ss.str());
		//
		return -10;
	}	
	//
	if(sCharOut.max_size()*sizeof(*sCharOut.c_str()) < sourceMappedFile.Residual()){
		std::stringstream ss;
		ss << m_sErrorHead << _T("max allowed string size in char:") << sCharOut.max_size()
			<< _T(" was < file size in char:") << sourceMappedFile.Residual()/sizeof(*sCharOut.c_str());
		sError.append(ss.str());
		//
		return -11;
	}
	//
	sCharOut.reserve(sourceMappedFile.Residual()/sizeof(buffer[0]));
	//
	__uint64 buflen = buf_size*sizeof(buffer[0]);
	while (sourceMappedFile.Residual()>0){		
		__uint64 l = sourceMappedFile.Read_bytes(buffer, buflen);
		buffer[l] = '\0';
		sCharOut.append(buffer);
	}
	//
	sourceMappedFile.Close();
	//
	return 1;
}

std::streamsize CBoostReadOnlyMapFile::CountFileLines(
	const _tstring & sSourceFile, 
	const TCHAR & cEOL,  
	_tstring & sError){
	//
	boost::filesystem::path pathSource = boost::filesystem::path(sSourceFile.c_str());
	//
	if(!boost::filesystem::exists(pathSource)){
		std::stringstream ss;
		ss << m_sErrorHead << _T("source file:") << sSourceFile << _T(" did not exist.");
		sError.append(ss.str());
		//
		return -1;
	}
	//
	if(boost::filesystem::is_directory(pathSource)){
		std::stringstream ss;
		ss << m_sErrorHead << _T("source file:") << sSourceFile << _T(" was a folder.");
		sError.append(ss.str());
		//
		return -2;
	}
	//METHOD I:
	//std::ifstream infstream(sSourceFile.c_str());
	////
	//std::streamsize nRC = std::count(std::istreambuf_iterator<TCHAR>(infstream), std::istreambuf_iterator<TCHAR>(), cEOL);
	////
	//if(infstream.is_open()){
	//	infstream.close();
	//}
	////
	//return nRC;
	//
	//METHOD II:
	//
	__uint64 nMax_Num_Pages = 1;
	CBoostReadOnlyMapFile sourceMappedFile(sSourceFile,
		nMax_Num_Pages);
	if(!sourceMappedFile.IsOpen()){
		std::stringstream ss;
		ss << m_sErrorHead << _T("could not open a maped file with source file:")
			<< sSourceFile;
		sError.append(ss.str());
		//
		return -10;
	}	
	try{
		return sourceMappedFile.Lines(cEOL);
	}
	catch(std::ios::failure & fail){
		std::stringstream ss;
		ss << m_sErrorHead << _T("could not get lines with source file:")
			<< sSourceFile << _T(" \nwith failure=")
			<< fail.what();
		sError.append(ss.str());
		//
		return -11;
	}
}

size_t CBoostReadOnlyMapFile::GetFileLines(
	const _tstring & sSourceFile, 
	std::vector<_tstring> & vectorAllLines,
	_tstring & sError){
	//
	boost::filesystem::path pathSource = boost::filesystem::path(sSourceFile.c_str());
	//
	if(!boost::filesystem::exists(pathSource)){
		std::stringstream ss;
		ss << m_sErrorHead << _T("source file:") << sSourceFile << _T(" did not exist.");
		sError.append(ss.str());
		//
		return -1;
	}
	//
	if(boost::filesystem::is_directory(pathSource)){
		std::stringstream ss;
		ss << m_sErrorHead << _T("source file:") << sSourceFile << _T(" was a folder.");
		sError.append(ss.str());
		//
		return -2;
	}
	//
	__uint64 nMax_Num_Pages = 1;
	CBoostReadOnlyMapFile sourceMappedFile(sSourceFile,
		nMax_Num_Pages);
	if(!sourceMappedFile.IsOpen()){
		std::stringstream ss;
		ss << m_sErrorHead << _T("could not open a maped file with source file:")
			<< sSourceFile;
		sError.append(ss.str());
		//
		return -10;
	}
	//
	sourceMappedFile.Lines(vectorAllLines);
	//
	sourceMappedFile.Close();
	//
	return vectorAllLines.size();
}

int CBoostReadOnlyMapFile::WriteFileCopy(
	const _tstring & sSourceFile, 
	const _tstring & sTargetFile, 
	const bool & bOverwrite,
	_tstring & sError){
	boost::filesystem::path pathSource = boost::filesystem::path(sSourceFile.c_str());
	boost::filesystem::path pathTarget = boost::filesystem::path(sTargetFile.c_str());
	if(!boost::filesystem::exists(pathSource)){
		std::stringstream ss;
		ss << m_sErrorHead << _T("source file:") << sSourceFile << _T(" did not exist.");
		sError.append(ss.str());
		//
		return -1;
	}
	//
	if(boost::filesystem::is_directory(pathSource)){
		std::stringstream ss;
		ss << m_sErrorHead << _T("source file:") << sSourceFile << _T(" was a folder.");
		sError.append(ss.str());
		//
		return -2;
	}
	//
	if(boost::filesystem::exists(pathTarget)){
		//
		if(boost::filesystem::is_directory(pathTarget)){
			std::stringstream ss;
			ss << m_sErrorHead << _T("target file:") << sTargetFile << _T(" was a folder.");
			sError.append(ss.str());
			//
			return -3;
		}
		//
		if(!bOverwrite){
			std::stringstream ss;
			ss << m_sErrorHead << _T("target file:") << sTargetFile << _T(" already existed.");
			sError.append(ss.str());
			//
			return -4;
		}
		else{
			boost::filesystem::remove(pathTarget);
		}
	}
	//
	__uint64 nMax_Num_Pages = 1;
	const int buf_size=1024;
	char buffer[buf_size+1];
	CBoostReadOnlyMapFile sourceMappedFile(sSourceFile,
		nMax_Num_Pages);
	if(!sourceMappedFile.IsOpen()){
		std::stringstream ss;
		ss << m_sErrorHead << _T("could not open a maped file with source file:")
			<< sSourceFile;
		sError.append(ss.str());
		//
		return -10;
	}
	//
	boost::iostreams::mapped_file_params paramsForTargetMappedFile(sTargetFile.c_str());
	paramsForTargetMappedFile.new_file_size = sourceMappedFile.Residual();
	boost::iostreams::stream<boost::iostreams::mapped_file_sink> targetMappedFileStream;
	targetMappedFileStream.open(boost::iostreams::mapped_file_sink(paramsForTargetMappedFile));
	//
	if(!targetMappedFileStream.is_open()){
		std::stringstream ss;
		ss << m_sErrorHead << _T("could not open a stream with target file:")
			<< sTargetFile;
		sError.append(ss.str());
		//
		return -11;
	}
	//
	__uint64 buflen = buf_size*sizeof(buffer[0]);
	while (sourceMappedFile.Residual()>0){		
		__uint64 l = sourceMappedFile.Read_bytes(buffer, buflen);
		buffer[l] = '\0';
		targetMappedFileStream << buffer;
	}
	//
	if(targetMappedFileStream.is_open()){
		targetMappedFileStream.close();
	}
	//
	sourceMappedFile.Close();
	//
	return 1;
}



