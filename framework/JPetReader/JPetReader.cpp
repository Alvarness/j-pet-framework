#include "JPetReader.h"
#include <cassert>
#include <iostream>

#include <TObjArray.h>
#include "../JPetUserInfoStructure/JPetUserInfoStructure.h"

JPetReader::JPetReader () : fBranch(0), fObject(0), fTree(0), fFile(NULL) {
}

JPetReader::JPetReader (const char* filename) : fBranch(0), fObject(0), fTree(0), fFile(NULL), m_TFile(filename, "READ")
{
  //if (OpenFile(filename) ) ReadData("");
}

JPetReader::~JPetReader () {
  CloseFile();
  closeFileUsingList();
}

void JPetReader::CloseFile () {
  if (fFile != NULL) {
    if (fFile->IsOpen()) fFile->Close();
    delete fFile;
    fFile = NULL;
  }
  fBranch = 0;
  fObject = 0;
  fTree = 0;
}

void JPetReader::closeFileUsingList()
{
  if(m_TFile.IsOpen())
  {
    m_TFile.cd();
    m_TFile.Close();
  }
}

bool JPetReader::OpenFile (const char* filename) {
  CloseFile();
  
  fFile = new TFile(filename);
  //fFile.OpenFile(filename);

  if ((!fFile->IsOpen())||fFile->IsZombie()) {
    ERROR("Cannot open file.");
    CloseFile();
    return false;
  }
  return true;
}

/* tu też bym zwrócił boola i wyszczególnił przypadek,
 * gdy nie ma obiektu o danej nazwie w pliku - Karol
 */
void JPetReader::ReadData (const char* objname) {
  assert(objname);
  //if (objname[0]== 0) { ///@warning nie rozumiem tego warunku
  //  ERROR("No object name specified");
  //  return;
  //}
  fTree = static_cast<TTree*>(fFile->Get(objname));
  assert(fTree);
  TObjArray* arr = fTree->GetListOfBranches();
  fBranch = (TBranch*)(arr->At(0));
  //fBranch = fTree->GetBranch(objname);
  assert(fBranch);
  fBranch->SetAddress(&fObject);
}

TObject* JPetReader::GetHeader(){
    // @todo The same as in writer. At() should take an enum that keeps positions of different things.
    return fTree->GetUserInfo()->At(JPetUserInfoStructure::kHeader);
}