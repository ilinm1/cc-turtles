Set of tools for building 3D models and images with computercraft turtles.<br />

### Tools:
img2vox - converts image to a vox file (using dithering to limit the amount of colors).<br />
series2vox - converts series of images to a vox file.<br />
quarry - creates a program for digging out a parallelepiped area.<br />
vox2bin - converts vox file to a binary file with turtle instructions.<br />

### .vox format:
".vox" file contains voxel model dimensions (X, Y, Z), material count and uncompressed voxel data (in that order).<br />
Each voxel is represented by a single byte, it's material number.<br />
Material numbers should be in contigious order, starting from one (zero means that voxel is empty).<br />
Each material in a model will be mapped to it's own block type when building.<br />
Layers are written starting from the bottom layer (lowest Z coordinate), while each layer is written starting from it's top left corner (highest Y and lowest X coordinate).<br />

### How to use:
1. Create a wired network containing a computer, some chests for storage and a full block wired modem in the same place where turtle will be restocking with materials.
2. Put some materials and fuel in the chests.
3. (Optional) If you want to have remote control over the turtle place another computer with a wireless modem and remember it's ID (driver will request it later). Also equip the turtle with a wireless modem.
4. Install "turtle-provider.lua" on the computer connected to the chests, "turtle-driver.lua" on the turtle and "turtle-controller.lua" on the computer with a wireless modem (lua files are in the "src-lua" directory).
5. Create a binary file with instructions for building your model and transfer it to the turtle (you can drag and drop the file into the minecraft's window).
6. Launch all programs, follow driver's instructions. For receiving messages and sending commands from/to the turtle use the turtle controller ("select TURTLE NUMBER", "status"/"pause"/"stop"/"jump INSTRUCTION").

### Troubleshooting:
Check that you've specified the right materials and fuel type when running the vox2bin program. If some materials are unavailable turtle will wait until it's request is fullfilled indefinitely (unless unpaused with the controller).<br />
Check that all your storage chests are connected to the wired network and that their modems are activated (right click on them).<br />
If a wired modem at the restock point is deactivating once the turtle is moving place some other peripheral adjacent to it and activate it again.<br />
Read messages received by the controller (if you've set it up).<br />
Read provider and driver logs ("provider-log.txt" and "turtle-log.txt").<br />
Feel free to make issues (or even better PRs (: ) if you encounter any problems.<br />

### Other tools:
Free mesh voxelizer, export as image series and use series2vox to convert it to vox file - https://www.drububu.com/miscellaneous/voxelizer/index.html<br />

### Screenshots:
<img width="1680" height="1050" alt="image" src="https://github.com/user-attachments/assets/e7fe61b5-5477-4826-a26f-2f6c715d3bc5" />
<img width="1680" height="1050" alt="image" src="https://github.com/user-attachments/assets/8f70ed01-6bd9-4777-b195-9f854564f0e5" />
