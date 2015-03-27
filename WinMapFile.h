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

#ifndef __CWINMAPFILE_H__
#define __CWINMAPFILE_H__

#ifdef WINDOWS
#ifdef UNICODE

#include "AtlBase.h"
#include "AtlConv.h"

///////////////////////////////////////////////////////
#include "FILEMAPHELPER_Export.h"
class CWinMapFile  
{
public:
                 CWinMapFile(LPCTSTR pPath,BOOL bIsText=FALSE,BOOL bIsReadOnly=TRUE);
  virtual       ~CWinMapFile();
public:
  void  Close();

  TCHAR * GetBase();
  unsigned long  GetFileSize();
  void           RevertFile();

protected:
  BOOL           Open(LPCTSTR pPath,BOOL bIsReadOnly,BOOL bIsText); 
  
  TCHAR * m_pBase;
  HANDLE         m_hFile;
  HANDLE         m_hFileMap;
  BOOL           m_bIsText;
  DWORD          m_dwFileSize;

private:
	CString ReturnLastError();
};

///////////////////////////////////////////////////////
#include "FILEMAPHELPER_Deport.h"

#endif //!UNICODE
#endif //!WINDOWS

#endif //!__CWINMAPFILE_H__