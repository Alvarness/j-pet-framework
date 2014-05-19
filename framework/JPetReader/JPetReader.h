/**
 *  @copyright Copyright (c) 2013, Wojciech Krzemien
 *  @file JPetReader.h 
 *  @author Wojciech Krzemien, wojciech.krzemien@if.uj.edu.pl
 *  @brief
 */ 

#ifndef JPETREADER_H 
#define JPETREADER_H 

#include <cstddef>

#include <TBranch.h>
#include <TFile.h>
#include <TNamed.h>
#include <TTree.h>
#include <vector>

#include "../../JPetLoggerInclude.h"
#include "../JPetKB/JPetKB.h"

class JPetReader {
 public:
  JPetReader();
  explicit JPetReader(const char* filename);
  virtual ~JPetReader();
  
  virtual void CloseFile();
  virtual void closeFileUsingList(void);
  virtual long long GetEntries () const { return fTree->GetEntries(); }
  virtual int GetEntry (int entryNo) {return fTree->GetEntry(entryNo); } /// the name of the function is bad but it mimics ROOT function 
  virtual bool OpenFile(const char* filename);
  virtual void ReadData(const char* objname = "");
  virtual TNamed& GetData () {return *fObject;}
  template <class T>
  void fillDataUsingList(std::vector<T> &p_container, const std::string &p_objectName);
  TObject* GetHeader();

 protected:
  TBranch* fBranch;
  TNamed* fObject;
  TTree* fTree;
  TFile* fFile;
  TFile m_TFile;

 private:
  JPetReader(const JPetReader&);
  void operator=(const JPetReader);

};

template <class T>
void JPetReader::fillDataUsingList(std::vector<T> &p_container, const std::string &p_objectName)
{
  TList *m_TList = (TList*)m_TFile.Get(p_objectName.c_str());
  
  TObject *obj;
  
  TIter next(m_TList);
  while(obj = next())
  {
    T *l_item = static_cast<T*>(obj);
    p_container.push_back(*l_item);
  }
}

#endif /*  !JPETREADER_H */
