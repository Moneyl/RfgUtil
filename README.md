# RfgUtil
This tool has several options for dealing with files used by Red Faction Guerrilla, such as packing/repacking vpp/str2 files and updating asm_pc files.

## How to use
Download RfgUtil from the [releases page](https://github.com/Moneyl/RfgUtil/releases/). See the zip file under the assets section.

## Unpacking vpp_pc|str2_pc files
Drag and drop the file onto the vpp_pc to extract it. If you pass the `-r` option to the command line it will also extract any str2_pc files within a vpp_pc.

**Options:**

`-r` = Recursively extract the packfile and its contents.

**Examples:**

This will extract mp_crashsite to a folder with the same name, and extract the str2_pc files within it.

```RfgUtil.exe .\mp_crashsite.vpp_pc -r```

This will extract mp_crashsite to the folder `.\out\`, and extract the str2_pc files within it.

```RfgUtil.exe .\mp_crashsite.vpp_pc .\out\ -r```


## Repacking vpp_pc|str2_pc files
Drag and drop a folder onto the exe to repack it into a vpp/str2. If the folder has an @streams.xml it'll be repacked into a str2_pc file, otherwise it'll be repacked into a .vpp_pc file. You can pass a second path to override this behavior. If the @streams.xml is present only the files in the xml will be packed, and the compressed/condensed value in the xml will be used.

**Options:**

`-c` = Compress the data.

`-e` = Condense the data.


**Examples:**

Pack the contents of the ns_base folder:

```RfgUtil.exe .\mp_crashsite\ns_base\```

Pack the contents of the mp_crashsite folder. The data will be both compressed and condensed.

```RfgUtil.exe .\mp_crashsite\ -ce```


## Modifying asm_pc files
Any time a str2_pc file is changed the related asm_pc must also be update. This tool can auto update them and provides a few other helpers. The edited str2_pc files must be in the same folder as the asm_pc file. 

**Options:**

`-u` - Update the contents of the asm_pc file. This will also be done if an asm_pc path is passed to the exe without any options.

`-l` - List all the containers inside the asm_pc.

`-p` - List all the primitives inside the asm_pc.

**Examples:**

Update the contents of the asm_pc based on the edited str2_pc files in the same folder.

```RfgUtil.exe .\mp_crashsite\mp_crashsite.asm_pc -u```

List the containers in the asm_pc.

```RfgUtil.exe .\mp_crashsite\mp_crashsite.asm_pc -l```

List the containers and primitives in the asm_pc.

```RfgUtil.exe .\mp_crashsite\mp_crashsite.asm_pc -lp```

Below is an example of moving an asset from one vpp to another. It moves the asset `0101crane_b.rfgchunkx` from `mp_crashsite.asm_pc` to `mp_crescent.asm_pc`.
- Recursively extract both vpp_pc files
- Get a list of files associated with the asset using: ```RfgUtil.exe .\mp_crashsite\mp_crashsite.asm_pc -lp``` Check which of the files are in ns_base for mp_crashsite, and copy them to ns_base for mp_crescent. Also add those files to @streams.xml in `.\mp_crescent\ns_base\`.
- Repack ns_base.str2_pc for mp_crescent.
- Copy the asset from mp_crashsite.asm_pc to mp_crescent.asm_pc using ```RfgUtil.exe -m .\mp_crescent\mp_crescent.asm_pc .\mp_crashsite\mp_crashsite.asm_pc 0101crane_b.rfgchunkx```
- Update the mp_crescent asm_pc using ```RfgUtil.exe .\mp_crescent\mp_crescent.asm_pc```.
- Repack mp_crescent.vpp_pc using ```RfgUtil.exe .\mp_crescent\```
