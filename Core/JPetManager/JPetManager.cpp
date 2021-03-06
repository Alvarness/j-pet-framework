/**
 *  @copyright Copyright 2016 The J-PET Framework Authors. All rights reserved.
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may find a copy of the License in the LICENCE file.
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  @file JPetManager.cpp
 */

#include "./JPetManager.h"

#include <cassert>
#include <string>
#include <exception>

#include "./JPetLoggerInclude.h"
#include "./JPetCommonTools/JPetCommonTools.h"
#include "./JPetCmdParser/JPetCmdParser.h"
#include "./JPetScopeLoader/JPetScopeLoader.h"
#include "./JPetUnzipAndUnpackTask/JPetUnzipAndUnpackTask.h"
#include "./JPetParamBankHandlerTask/JPetParamBankHandlerTask.h"
#include "./JPetOptionsGenerator/JPetOptionsGenerator.h"

#include <TThread.h>

using namespace jpet_options_tools;

JPetManager::JPetManager()
{
  fTaskGeneratorChain = new TaskGeneratorChain;
}

JPetManager& JPetManager::getManager()
{
  static JPetManager instance;
  return instance;
}

bool JPetManager::areThreadsEnabled() const
{
  return fThreadsEnabled;
}

void JPetManager::setThreadsEnabled(bool enable)
{
  fThreadsEnabled = enable;
}

bool JPetManager::run(int argc, const char** argv)
{
  if (!parseCmdLine(argc, argv)) {
    ERROR("While parsing command line arguments");
    return false;
  }
  INFO( "======== Starting processing all tasks: " + JPetCommonTools::getTimeString() + " ========\n" );
  std::vector<JPetTaskChainExecutor*> executors;
  std::vector<TThread*> threads;
  auto inputDataSeq = 0;
  /// For every input option, new TaskChainExecutor is created, which creates the chain of previously
  /// registered tasks. The inputDataSeq is the identifier of given chain.
  for (auto opt : fOptions) {
    JPetTaskChainExecutor* executor = new JPetTaskChainExecutor(fTaskGeneratorChain, inputDataSeq, opt.second);
    executors.push_back(executor);
    if (areThreadsEnabled()) {
      auto thr = executor->run();
      if (thr) {
        threads.push_back(thr);
      } else {
        ERROR("thread pointer is null");
      }
    } else {
      if (!executor->process()) {
        ERROR("While running process");
        return false;
      }
    }
    inputDataSeq++;
  }
  if (areThreadsEnabled()) {
    for (auto thread : threads) {
      assert(thread);
      thread->Join();
    }
  }
  for (auto& executor : executors) {
    if (executor) {
      delete executor;
      executor = 0;
    }
  }
  INFO( "======== Finished processing all tasks: " + JPetCommonTools::getTimeString() + " ========\n" );
  return true;
}

bool JPetManager::parseCmdLine(int argc, const char** argv)
{
  auto addDefaultTasksFromOptions = [&](const std::map<std::string, boost::any>& options) {
    auto fileType = FileTypeChecker::getInputFileType(options);
    if (fileType == FileTypeChecker::kScope) {
      auto task2 = []() {
        return new JPetScopeLoader(std::unique_ptr<JPetScopeTask>(new JPetScopeTask("JPetScopeReader")));
      };
      fTaskGeneratorChain->insert(fTaskGeneratorChain->begin(), task2);
    }
    auto paramBankHandlerTask = []() {
      return new JPetParamBankHandlerTask("ParamBank Filling");
    };
    fTaskGeneratorChain->insert(fTaskGeneratorChain->begin(), paramBankHandlerTask);
    /// add task to unzip or unpack if needed
    auto task = []() {
      return new JPetUnzipAndUnpackTask("UnpackerAndUnzipper");
    };
    fTaskGeneratorChain->insert(fTaskGeneratorChain->begin(), task);
  };

  try {
    JPetOptionsGenerator optionsGenerator;
    JPetCmdParser parser;
    auto optionsFromCmdLine = parser.parseCmdLineArgs(argc, argv);
    /// One  common map of all options
    auto allValidatedOptions = optionsGenerator.generateAndValidateOptions(optionsFromCmdLine);
    addDefaultTasksFromOptions(allValidatedOptions);

    int numberOfRegisteredTasks = 1;
    if (fTaskGeneratorChain) {
      numberOfRegisteredTasks = fTaskGeneratorChain->size();
    }
    fOptions = optionsGenerator.generateOptionsForTasks(allValidatedOptions, numberOfRegisteredTasks);
  } catch (std::exception& e) {
    ERROR(e.what());
    return false;
  }
  return true;
}

//
JPetManager::~JPetManager(){}


void JPetManager::useTask(const char* name, const char* inputFileType, const char* outputFileType)
{
  assert(fTaskGeneratorChain);
  if ( fTasksDictionary.count(name) > 0 ) {
    TaskGenerator userTaskGen = fTasksDictionary.at(name);
    // wrap the JPetUserTask-based task in a JPetTaskIO
    fTaskGeneratorChain->push_back( [name, inputFileType, outputFileType, userTaskGen]() {
      JPetTaskIO* task = new JPetTaskIO(name, inputFileType, outputFileType);
      task->addSubTask(std::unique_ptr<JPetTaskInterface>(userTaskGen()));
      return task;
    });
  } else {
    ERROR(Form("The requested task %s is unknown", name));
    exit(1);
  }
}

JPetManager::Options JPetManager::getOptions() const
{
  return fOptions;
}

void JPetManager::clearRegisteredTasks()
{
  delete fTaskGeneratorChain;
  fTaskGeneratorChain = new TaskGeneratorChain;
}