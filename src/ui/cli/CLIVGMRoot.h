/**
 * VGMTrans (c) - 2002-2021
 * Licensed under the zlib license
 * See the included LICENSE for more information
 */

#pragma once

#include "Root.h"
#include <set>
#include <filesystem>

#define CLI_APP_NAME "vgmtrans"

namespace fs = std::filesystem;

class CLIVGMRoot : public VGMRoot {

public:

  size_t GetNumCollections() {
    return vVGMColl.size();
  }

  void DisplayUsage();

  void DisplayHelp();

  bool MakeOutputDir();

  bool ExportAllCollections();

  bool ExportCollection(VGMColl* coll);

  bool SaveMidi(VGMColl *coll);

  bool SaveSF2(VGMColl *coll);

  bool SaveDLS(VGMColl *coll);

  bool OpenRawFile(const std::string &filename) override;

  virtual bool Init();

  virtual void UI_SetRootPtr(VGMRoot** theRoot);

  virtual void UI_Exit() {}

  virtual void UI_Log(LogItem* theLog);

  virtual void UpdateCollections();

  virtual std::string UI_GetOpenFilePath(const std::string& suggestedFilename = "",
                                          const std::string& extension = "");

  virtual std::string UI_GetSaveFilePath(const std::string& suggestedFilename,
                                          const std::string& extension = "");

  virtual std::string UI_GetSaveDirPath(const std::string& suggestedDir = "");

  std::set<fs::path> inputFiles = {};
  fs::path outputDir = fs::path(".");
};

extern CLIVGMRoot cliroot;