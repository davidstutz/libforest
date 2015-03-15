#include <iostream>
#include "libforest/libforest.h"
#include <chrono>
#include <fstream>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

using namespace libf;

/**
 * Example of Random Forest learning. Use for example MNIST of USPS datasets,
 * however, make sure to convert to DAT files first (see cli_convert --help).
 * 
 * **The original MNIST file format is currently not supported.**
 * 
 * Usage:
 * 
 * $ ./examples/cli_rf --help
 * Allowed options:
 *   --help                   produce help message
 *   --file-train arg         path to train DAT file
 *   --file-test arg          path to test DAT file
 *   --num-features arg (=10) number of features to use (set to dimensionality of 
 *                            data to learn deterministically)
 *   --use-bootstrap          use bootstrapping for training
 *   --num-trees arg (=100)   number of trees in forest
 *   --max-depth arg (=100)   maximum depth of trees
 *   --num-threads arg (=1)   number of threads for learning
 * 
 */
int main(int argc, const char** argv)
{
    boost::program_options::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("file-train", boost::program_options::value<std::string>(), "path to train DAT file")
        ("file-test", boost::program_options::value<std::string>(), "path to test DAT file")
        ("num-features", boost::program_options::value<int>()->default_value(10), "number of features to use (set to dimensionality of data to learn deterministically)")
        ("use-bootstrap", "use bootstrapping for training")
        ("num-trees", boost::program_options::value<int>()->default_value(100), "number of trees in forest")
        ("max-depth", boost::program_options::value<int>()->default_value(100), "maximum depth of trees")
        ("num-threads", boost::program_options::value<int>()->default_value(1), "number of threads for learning");

    boost::program_options::positional_options_description positionals;
    positionals.add("file-train", 1);
    positionals.add("file-test", 1);
    
    boost::program_options::variables_map parameters;
    boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(desc).positional(positionals).run(), parameters);
    boost::program_options::notify(parameters);

    if (parameters.find("help") != parameters.end())
    {
        std::cout << desc << std::endl;
        return 1;
    }
    
    boost::filesystem::path trainDat(parameters["file-train"].as<std::string>());
    if (!boost::filesystem::is_regular_file(trainDat))
    {
        std::cout << "Train DAT file does not exist at the specified location." << std::endl;
        return 1;
    }
    
    boost::filesystem::path testDat(parameters["file-test"].as<std::string>());
    if (!boost::filesystem::is_regular_file(testDat))
    {
        std::cout << "Test DAT file does not exist at the specified location." << std::endl;
        return 1;
    }
    
    const bool useBootstrap = parameters.find("use-bootstrap") != parameters.end();
    
    DataStorage::ptr storage = DataStorage::Factory::create();
    DataStorage::ptr storageT = DataStorage::Factory::create();
    
    LibforestDataReader reader;
    reader.read(trainDat.string(), storageT);
    reader.read(testDat.string(), storage);
    
    // Important for sorted datasets!
    storageT->randPermute();
    
    std::cout << "Training Data" << std::endl;
    storageT->dumpInformation();
    
    
    RandomForestLearner forestLearner;
    forestLearner.addCallback(RandomForestLearner::defaultCallback, 1);
    
    forestLearner.getTreeLearner().setUseBootstrap(useBootstrap);
    forestLearner.getTreeLearner().setMaxDepth(parameters["max-depth"].as<int>());
    forestLearner.getTreeLearner().setNumFeatures(parameters["num-features"].as<int>());
    
    forestLearner.setNumTrees(parameters["num-trees"].as<int>());
    forestLearner.setNumThreads(parameters["num-threads"].as<int>());
    
    RandomForest::ptr forest = forestLearner.learn(storageT);
    
    AccuracyTool accuracyTool;
    accuracyTool.measureAndPrint(forest, storage);
    
    ConfusionMatrixTool confusionMatrixTool;
    confusionMatrixTool.measureAndPrint(forest, storage);
    
//    VariableImportanceTool variableImportanceTool;
//    variableImportanceTool.measureAndPrint(&forestLearner);
    
    return 0;
}
