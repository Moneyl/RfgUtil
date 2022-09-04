#include "common/Typedefs.h"
#include "Common/timing/Timer.h"
#include "Common/filesystem/Path.h"
#include "Common/filesystem/File.h"
#include "common/string/String.h"
#include "common/Defer.h"
#include <RfgTools++/formats/packfiles/Packfile3.h>
#include <cxxopts.hpp>
#include <iostream>

#pragma warning(disable:4005) //Disable repeat definitions for windows defines
//Target windows 7 with win10 sdk
#include <winsdkver.h>
#define _WIN32_WINNT 0x0601
#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#pragma warning(default:4005)

cxxopts::Options options("RfgUtil", "A set of utilities for file formats used by Red Faction Guerrilla. Note that the options only apply to certain file formats/tools. That's listed at the start of each options description.");
std::vector<string> InputPaths; //First argument passed. Folder or file to operate on. Tool to use is determined automatically based on if it's a file or folder, and the file extension.
string OutputPath;
bool Verbose = false;
bool RequireEnterToExit = false;

//Tools built into the exe. Selected based on first path passed to exe. Checks if it's a folder or file and the file extension.
void PackfileUnpacker(cxxopts::ParseResult& args, const string& inputPath);
void PackfilePacker(cxxopts::ParseResult& args, const string& inputPath);
void AsmPcUpdater(cxxopts::ParseResult& args, const string& inputPath);

//Exits program with specified result. Requires user to press enter before closing the window if they're using drag and drop. That way users can see any errors instead of it instantly closing
void ErrorExit();
void SuccessExit();
string GetExeFolder();

//Update contents an asm_pc file based on the str2_pc files in the same folder. You must unpack, edit, and repack the str2_pc files first.
void UpdateAsmPc(const string& asmPath);
void UpdateAsmPc(AsmFile5& asmFile, const string& asmPath);

const string RFG_UTIL_VERSION = "v1.1.0";
const string AdditionalHelp = 
R"(Some things to note:
    - To unpack a vpp_pc or str2_pc file drag and drop it onto the exe or pass it as the first command line option.
    - To repack a vpp_pc or str2_pc file drag and drop the folder onto the exe. If the folder as an @streams.xml file it'll repack it into a str2_pc. Otherwise it'll repack into a vpp_pc.
    - Drag and drop an asm_pc file onto the exe to auto update it's contents. The changed str2_pc files must be in the same folder as the asm_pc file. If you pass any other asm_pc arguments like -l, -p, or -m then the asm_pc won't be updated unless you also pass -u.
    - Some options like manually setting the compressed/condensed repacking options or copying containers between asm_pc files are only available with through the command line. Drag and drop is for convenience in simple use cases.

Command line options:)";

int main(int argc, char* argv[])
{
    try
    {
        //Init cmdline options
        options
            .set_width(120)
            .custom_help(AdditionalHelp)
            .add_options()
            ("c,compressed", "[packing .vpp_pc] Compress packfile while repacking. When packing str2_pc files the @streams.xml file determines this.", cxxopts::value<bool>()->default_value("false"))
            ("e,condensed", "[packing .vpp_pc] Condense packfile while repacking. When packing str2_pc files the @streams.xml file determines this.", cxxopts::value<bool>()->default_value("false"))
            ("r,recursive", "[unpacking .vpp_pc|.str2_pc] Recursively unpack packfile. Meaning the packfile will be extracted & any str2_pc files inside it will be extracted", cxxopts::value<bool>()->default_value("false"))
            ("l,containers", "[.asm_pc] List containers in an asm_pc file. If this is passed the asm_pc won't be modified. Containers will also be listed if -p|--primitives is passed.", cxxopts::value<bool>()->default_value("false"))
            ("p,primitives", "[.asm_pc] List primitives in an asm_pc file + the containers they're in. If this is passed the asm_pc won't be modified.", cxxopts::value<bool>()->default_value("false"))
            ("m,move", "[.asm_pc] Move container from one asm_pc to another. Place the source asm_pc path and the container name after this argument. E.g. RfgUtil.exe destination.asm_pc -m source.asm_pc containerName. Note that just using this command isn't enough. You should also copy the relevant assets to the destination str2_pc, repack it, and use the -u|--update argument to update the asm_pc based on the packed str2_pc.", cxxopts::value<bool>()->default_value("false"))
            ("u,update", "[.asm_pc] Update the contents of the asm_pc using the str2_pc files in the same folder. This is performed last if any other asm_pc arguments like -l, -p, or -m are passed. If you pass an asm_pc file and no arguments it'll also update the asm_pc.", cxxopts::value<bool>()->default_value("false"))
            ("input", "", cxxopts::value<std::vector<string>>()->default_value(""))
            ("output", "", cxxopts::value<string>()->default_value(""))
            ("container", "", cxxopts::value<string>()->default_value(""))
            ("v,verbose", "Log more status updates to the console.", cxxopts::value<bool>()->default_value("false"))
            ;
        options.parse_positional({ "input", "output", "container" });
        cxxopts::ParseResult args = options.parse(argc, argv);

        //TODO: Support passing multiple files/folders to the tool. Currently the first argument must be the file/folder & any others are ignored
        //Extract file/folder path from unmatched args
        bool foundInputPath = false;
        if (args["input"].as<std::vector<string>>().size() != 0)
        {
            InputPaths = args["input"].as<std::vector<string>>();
            if (InputPaths.size() == 1 && InputPaths[0] == "")
                foundInputPath = false; //If not input path is entered it'll sometimes stick just an empty string in the list
            else
                foundInputPath = true;
        }
        if (args["output"].as<string>() != "")
            OutputPath = args["output"].as<string>();

        if (args.arguments().size() == 0 && !foundInputPath) //Print help message if no args passed
        {
            printf("\nNo options passed.\n");
            printf("%s\n", options.help({ "", "Group" }).c_str());
            RequireEnterToExit = true;
            SuccessExit();
        }
        else
        {
            //Don't require user to press enter to close program when they've passed arguments. That way it's not needed when using command line or batch scripts. Would be annoying. Required otherwise so drag & drop users see help/errors.
            RequireEnterToExit = false;
        }

        Verbose = args["verbose"].as<bool>();
        if (Verbose)
        {
            printf("Arg count: %zd\n", args.arguments().size());
            for (auto arg : args.arguments())
                printf("Arg: %s = %s\n", arg.key().c_str(), arg.value().c_str());
        }

        //Figure out which tool to use based on InputPath
        for (string inputPath : InputPaths)
        {
            if (std::filesystem::is_regular_file(inputPath))
            {
                string extension = Path::GetExtension(inputPath);
                printf("Detected file path: '%s'. Extension = '%s'\n", inputPath.c_str(), extension.c_str());
                if (extension == ".vpp_pc" || extension == ".str2_pc")
                {
                    printf("Packfile detected. Extracting '%s'...\n", inputPath.c_str());
                    PackfileUnpacker(args, inputPath);
                }
                else if (extension == ".asm_pc")
                {
                    printf("asm_pc file detected. Updating '%s'...\n", inputPath.c_str());
                    AsmPcUpdater(args, inputPath);
                }
                else
                {
                    printf("Unsupported file format '%s'. Exiting...", extension.c_str());
                    ErrorExit();
                }
            }
            else if (std::filesystem::is_directory(inputPath))
            {
                printf("Folder detected. Packing '%s'...\n", inputPath.c_str());
                PackfilePacker(args, inputPath);
            }
            else
            {
                printf("Failed to parse path '%s'. Please make sure it's either a folder or a file path. Exiting...", inputPath.c_str());
                ErrorExit();
            }
        }

    }
    catch (std::exception& ex)
    {
        printf("Exception thrown: '%s'\n", ex.what());
        ErrorExit();
    }

    SuccessExit();
}

void PackfileUnpacker(cxxopts::ParseResult& args, const string& inputPath)
{
    string packfileOutputPath = "";
    if (OutputPath == "")
        packfileOutputPath = Path::GetParentDirectory(inputPath) + "\\Unpack\\" + Path::GetFileNameNoExtension(inputPath) + "\\";
    else
        packfileOutputPath = OutputPath + "\\";

    std::filesystem::create_directories(packfileOutputPath);
    bool writeStreamsFile = (Path::GetExtension(inputPath) == ".str2_pc");
    if (Verbose)
        printf("packfileOutputPath: %s, writeStreamsFile: %s\n", packfileOutputPath.c_str(), writeStreamsFile ? "true" : "false");

    Packfile3 packfile(inputPath);
    packfile.ReadMetadata();
    packfile.ExtractSubfiles(packfileOutputPath + "\\", writeStreamsFile);

    //Extract str2_pc files inside packfile if passed recursive option.
    if (args["recursive"].as<bool>())
    {
        for (auto& entry : std::filesystem::directory_iterator(packfileOutputPath))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".str2_pc")
            {
                printf("Extracting %s...\n", entry.path().filename().string().c_str());
                string str2OutputPath = packfileOutputPath + "\\" + Path::GetFileNameNoExtension(entry.path().string()) + "\\";
                if (Verbose)
                    printf("str2OutputPath: %s\n", str2OutputPath.c_str());

                Packfile3 str2(entry.path().string());
                str2.ReadMetadata();
                str2.ExtractSubfiles(str2OutputPath, true);
            }
        }
    }
}

void PackfilePacker(cxxopts::ParseResult& args, const string& inputPath)
{
    bool compressed = args["compressed"].as<bool>();
    bool condensed = args["condensed"].as<bool>();

    string packfileOutputPath = "";
    if (OutputPath == "") //Write packfile to the parent folder if no custom output path is provided
    {
        //Get name of str2_pc file based on folder name. E.g. InputPath = ./mp_crashsite/mp_crashsite_map/ then str2Name = mp_crashsite_map
        string str2Name = std::filesystem::path(inputPath + "\\").parent_path().filename().string();
        if (str2Name.ends_with("\\") || str2Name.ends_with("/"))
            str2Name.pop_back();

        bool isStr2 = std::filesystem::exists(inputPath + "\\@streams.xml");
        packfileOutputPath = std::filesystem::path(inputPath + "\\").parent_path().parent_path().string() + "\\" + str2Name + (isStr2 ? ".str2_pc" : ".vpp_pc");
    }
    else
        packfileOutputPath = OutputPath;

    if (Verbose)
    {
        printf("InputPath: %s\n", (inputPath + "\\").c_str());
        printf("OutputPath: %s\n", packfileOutputPath.c_str());
    }

    Packfile3::Pack(inputPath + "\\", packfileOutputPath, compressed, condensed);
}

void AsmPcUpdater(cxxopts::ParseResult& args, const string& inputPath)
{
    //Default option is to update asm_pc. For easy update via drag and drop
    if (Verbose)
        printf("InputPath: %s\n", inputPath.c_str());

    if (args.arguments().size() == 0)
        UpdateAsmPc(inputPath);
    else
    {
        AsmFile5 asmFile;
        BinaryReader reader(inputPath);
        asmFile.Read(reader, Path::GetFileName(inputPath));

        //List containers
        if (args["containers"].as<bool>() || args["primitives"].as<bool>()) //Containers must also be listed if primitives are listed
        {
            printf("Containers in %s:\n", asmFile.Name.c_str());
            for (AsmContainer& container : asmFile.Containers)
            {
                printf("  %s", container.Name.c_str());
                if (!args["primitives"].as<bool>())
                    printf("\n");
                else
                {
                    //List primitives in container
                    printf(":\n");
                    for (AsmPrimitive& primitive : container.Primitives)
                        printf("    %s\n", primitive.Name.c_str());
                }
            }
        }

        //Move container & its primitives from one asm_pc to another
        if (args["move"].as<bool>())
        {
            string sourceAsmPath = OutputPath;
            string containerName = args["container"].as<string>();
            printf("Copying %s from '%s' to '%s'\n", containerName.c_str(), sourceAsmPath.c_str(), inputPath.c_str());
            if (std::filesystem::exists(sourceAsmPath))
            {
                //Load source asm_pc file
                AsmFile5 source;
                BinaryReader reader(sourceAsmPath);
                source.Read(reader, Path::GetFileName(sourceAsmPath));

                AsmContainer* sourceContainer = source.GetContainer(containerName);
                if (sourceContainer)
                {
                    //Copy source container + primitives to destination asm_pc (the one at InputPath)
                    AsmContainer newContainer(*sourceContainer); //Using compiler generated copy constructor. Works fine since AsmContainer and AsmPrimitive only have primitive fields, no pointers.
                    asmFile.Containers.push_back(newContainer);
                    asmFile.ContainerCount = asmFile.Containers.size();
                }
                else //Source asm_pc doesn't container specified container
                {
                    printf("'%s' doesn't have a container named '%s'\n", sourceAsmPath.c_str(), containerName.c_str());
                    ErrorExit();
                }

                asmFile.Write(inputPath);
            }
            else //Source asm_pc doesn't exist
            {
                printf("Source asm_pc passed with -m|--move doesn't exist. Exiting... Path: '%s'\n", sourceAsmPath.c_str());
                ErrorExit();
            }
        }

        //Update asm_pc contents last if -u was passed along with other asm_pc arguments
        if (args["update"].as<bool>())
        {
            UpdateAsmPc(asmFile, inputPath);
        }
    }
}

void ErrorExit()
{
    printf("\nError!");
    if (RequireEnterToExit)
    {
        printf("Press enter to close...\n");
        getchar(); //Returns once enter key is pressed
    }

    exit(EXIT_FAILURE);
}

void SuccessExit()
{
    printf("\nDone!");
    if (RequireEnterToExit)
    {
        printf("Press enter to close...\n");
        getchar(); //Returns once enter key is pressed
    }

    exit(EXIT_SUCCESS);
}

void UpdateAsmPc(const string& asmPath)
{
    AsmFile5 asmFile;
    BinaryReader reader(asmPath);
    asmFile.Read(reader, Path::GetFileName(asmPath));
    UpdateAsmPc(asmFile, asmPath);
}

void UpdateAsmPc(AsmFile5& asmFile, const string& asmPath)
{
    //Update containers
    for (auto& file : std::filesystem::directory_iterator(Path::GetParentDirectory(asmPath)))
    {
        if (!file.is_regular_file() || file.path().extension() != ".str2_pc")
            continue;

        //Parse container file to get up to date values
        string packfilePath = file.path().string();
        if (!std::filesystem::exists(packfilePath))
            continue;

        Packfile3 packfile(packfilePath);
        packfile.ReadMetadata();

        for (AsmContainer& container : asmFile.Containers)
        {
            struct AsmPrimitiveSizeIndices { size_t HeaderIndex = -1; size_t DataIndex = -1; };
            std::vector<AsmPrimitiveSizeIndices> sizeIndices = {};
            size_t curPrimSizeIndex = 0;
            //Pre-calculate indices of header/data size for each primitive in Container::PrimitiveSizes
            for (AsmPrimitive& prim : container.Primitives)
            {
                AsmPrimitiveSizeIndices& indices = sizeIndices.emplace_back();
                indices.HeaderIndex = curPrimSizeIndex++;
                if (prim.DataSize == 0)
                    indices.DataIndex = -1; //This primitive has no gpu file and so it only has one value in PrimitiveSizes
                else
                    indices.DataIndex = curPrimSizeIndex++;
            }
            const bool virtualContainer = container.CompressedSize == 0 && container.DataOffset == 0;

            if (!virtualContainer && String::EqualIgnoreCase(Path::GetFileNameNoExtension(packfile.Name()), container.Name))
            {
                container.CompressedSize = packfile.Header.CompressedDataSize;

                size_t dataStart = 0;
                dataStart += 2048; //Header size
                dataStart += packfile.Entries.size() * 28; //Each entry is 28 bytes
                dataStart += BinaryWriter::CalcAlign(dataStart, 2048); //Align(2048) after end of entries
                dataStart += packfile.Header.NameBlockSize; //Filenames list
                dataStart += BinaryWriter::CalcAlign(dataStart, 2048); //Align(2048) after end of file names

                printf("  Setting container offset '%s', packfile name: '%s'\n", container.Name.c_str(), packfile.Name().c_str());
                container.DataOffset = dataStart;
            }

            for (size_t entryIndex = 0; entryIndex < packfile.Entries.size(); entryIndex++)
            {
                Packfile3Entry& entry = packfile.Entries[entryIndex];
                for (size_t primitiveIndex = 0; primitiveIndex < container.Primitives.size(); primitiveIndex++)
                {
                    AsmPrimitive& primitive = container.Primitives[primitiveIndex];
                    AsmPrimitiveSizeIndices& indices = sizeIndices[primitiveIndex];
                    if (String::EqualIgnoreCase(primitive.Name, packfile.EntryNames[entryIndex]))
                    {
                        //If DataSize = 0 assume this primitive has no gpu file
                        if (primitive.DataSize == 0)
                        {
                            primitive.HeaderSize = entry.DataSize; //Uncompressed size
                            if (!virtualContainer)
                            {
                                container.PrimitiveSizes[indices.HeaderIndex] = primitive.HeaderSize; //TODO: Just remove PrimitiveSizes from AsmFile and generate it on write
                            }
                        }
                        else //Otherwise assume primitive has cpu and gpu file
                        {
                            primitive.HeaderSize = entry.DataSize; //Cpu file uncompressed size
                            primitive.DataSize = packfile.Entries[entryIndex + 1].DataSize; //Gpu file uncompressed size
                            if (!virtualContainer)
                            {
                                container.PrimitiveSizes[indices.HeaderIndex] = primitive.HeaderSize; //TODO: Just remove PrimitiveSizes from AsmFile and generate it on write
                                container.PrimitiveSizes[indices.DataIndex] = primitive.DataSize; //TODO: Just remove PrimitiveSizes from AsmFile and generate it on write
                            }
                        }
                    }
                }
            }
        }
    }

    asmFile.Write(asmPath);
}

string GetExeFolder()
{
    char path[FILENAME_MAX];
    if (GetModuleFileNameA(NULL, path, FILENAME_MAX) == 0)
        throw std::runtime_error("Failed to get exe folder. Error code: " + std::to_string(GetLastError()));

    return Path::GetParentDirectory(string(path));
}