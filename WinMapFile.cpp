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

#ifdef WINDOWS
#ifdef UNICODE


#include "atlstr.h"

#include "WinMapFile.h"


inline TCHAR * CWinMapFile::GetBase()
{
  return m_pBase;
}

inline unsigned long   CWinMapFile::GetFileSize()
{
  return m_dwFileSize;
}

CWinMapFile::CWinMapFile(LPCTSTR pPath,BOOL bIsText,BOOL bIsReadOnly)
       : m_pBase(0)
       , m_bIsText(bIsText)
       , m_hFile(0)
       , m_dwFileSize(0)
{
  Open(pPath,bIsReadOnly,m_bIsText);
}

CWinMapFile::~CWinMapFile()
{
  Close();
}

BOOL CWinMapFile::Open(LPCTSTR pPath,BOOL bIsReadOnly,BOOL bIsText)
{
	//tell the file size
	/*long totalFileSize;
	FILE *fpRead;
	fpRead = fopen(lpszPathName, "r");
	fseek (fpRead, 0L, SEEK_SET);
    fseek (fpRead, 0, SEEK_END);
    totalFileSize = ftell (fpRead);
    fseek (fpRead, 0L, SEEK_SET);
	if(totalFileSize<=0){
		return FALSE;
	}
	fclose(fpRead);*/
    // open file on disk  
	//Modify path
	CString path=_T("\\\\?\\");
	int i=0;
	while(pPath[i]!=_T('\0')){
        if(pPath[i]==_T('\\'))
			path+=_T("\\\\");
		else
			path+=pPath[i];
		i++;
	}
	//
    DWORD dwFile = bIsReadOnly ? GENERIC_READ : (GENERIC_READ|GENERIC_WRITE);
	//
	DWORD dwShare=bIsReadOnly ? FILE_SHARE_READ : FILE_SHARE_READ | FILE_SHARE_WRITE;
	//
	DWORD dwDispose = bIsReadOnly ? OPEN_EXISTING : CREATE_ALWAYS;
	// create file mapping  
    DWORD dwAccess = bIsReadOnly ? PAGE_READONLY : PAGE_READWRITE;
	//
    m_hFile = CreateFile(pPath,dwFile,dwShare,NULL,dwDispose,FILE_ATTRIBUTE_NORMAL,NULL);
    if (m_hFile == INVALID_HANDLE_VALUE)  
      return FALSE;  
    m_dwFileSize = ::GetFileSize(m_hFile,NULL);
    
    
    // create file mapping  
    m_hFileMap = CreateFileMapping(m_hFile,NULL,dwAccess,0,/*m_dwFileSize+(bIsText? 1 : 0)*/0,0);	
  
	if (!m_hFileMap)
	{
        CloseHandle(m_hFile);
        return FALSE;
	}

    //map view of file
    DWORD viewFlag = bIsReadOnly ? FILE_MAP_READ : FILE_MAP_WRITE;
    //get m_pBase 
    LPSTR lpstr=(LPSTR)MapViewOfFile(m_hFileMap,viewFlag,0,0,0);
    m_pBase = SysAllocStringLen(NULL, lstrlenA(lpstr));
    ::MultiByteToWideChar(CP_ACP, 0, lpstr, lstrlenA(lpstr), m_pBase, lstrlenA(lpstr));
	//
    if (!m_pBase)
	{
        CloseHandle(m_hFileMap);
        CloseHandle(m_hFile);
        return FALSE;
	}
    //write terminating null if opened in text mode.
    /*if (bIsText)          
	{
        DWORD ret=0;
        DWORD old=0;

        if (bIsReadOnly)
            ret = VirtualProtect(m_pBase+m_dwFileSize,1,PAGE_READWRITE,&old);
        m_pBase[m_dwFileSize] = '\0';
        if (bIsReadOnly)
            ret = VirtualProtect(m_pBase+m_dwFileSize,1,PAGE_READONLY,&old);
	}*/    
    return TRUE;
}

void CWinMapFile::Close()
{
  if (m_pBase)
  {
    // if text mode - make file shorter
    /*if (m_bIsText)
    {
      SetFilePointer(m_hFile,m_dwFileSize,NULL,FILE_BEGIN);
      SetEndOfFile(m_hFile);
    }*/
    UnmapViewOfFile(m_pBase);
    CloseHandle(m_hFileMap);
    CloseHandle(m_hFile);
	//... when done, free m_pBase
    ::SysFreeString(m_pBase);
    m_pBase = 0;
  }
}

void  CWinMapFile::RevertFile()
{
  if (m_pBase)
  {
    _tcsrev(m_pBase);

    LPSTR lpch = strchr((LPSTR)m_pBase,'\n');
    while(lpch)
    {

      if (*(lpch+1) == '\r')
      {
        *lpch++ = '\r';
        *lpch++ = '\n';
      }
      else
         lpch += 2;

      lpch = strchr(lpch,'\n');
    }
  }
}

CString CWinMapFile::ReturnLastError()
{
	LPVOID lpMsgBuf;
	CString s;
	DWORD code=GetLastError();
    FormatMessage( 
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            code,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL 
    );
	s.Format(_T("Error=%d:%s"),code, (LPCTSTR)lpMsgBuf);
    LocalFree( lpMsgBuf );
	//
	return s;
}

#endif //!UNICODE
#endif //!WINDOWS

