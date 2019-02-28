# Vault Streaming Model Format

## Header Format
<table>
  <tr><td>Size (bytes)</td><td>Type</td><td>Job</td></tr>
  <tr><td>4</td><td>Char</td><td>4CC "VSMF"</td></tr>
  <tr><td>2</td><td>Unsigned Int16</td><td>Version ID (Latest Published Version: 1)</td></tr>
  <tr><td>2</td><td>Bitflag Int16</td><td>What data is included in this file (See below)</td></tr>
  <tr><td>2</td><td>Unsigned Int16</td><td>How many materials in this file</td></tr>
  <tr><td>2</td><td>Unsigned Int16</td><td>How many meshes are in this file</td></tr>
  <tr><td>2</td><td>Unsigned Int16</td><td>How many textures are in this file</td></tr>
  <tr><td>2</td><td>Unsigned Int16</td><td>RESERVED. 0 in version 1</td></tr>
  <tr><td>24</td><td>2x Float Vec3</td><td>Model limits, min (x,y,z) and then max (x,y,z)</td></tr>
</table>

### Model Flags (enabled bits)
<table>
  <tr><td>& Flag</td><td>Included Feature</td></tr>
  <tr><td>0x1</td><td>Animation - There is animation table at the end of the file and immediately following this header is an animation header</td></tr>
</table>

### Animation Header (if Model Flag 0x1)
<table>
<tr><td>Size</td><td>Type</td><td>Value</td></tr>
<tr><td>2 bytes</td><td>Uint16_t</td><td>Animation Count</td></tr>
<tr><td>1 byte</td><td>Uint8_t</td><td>Bone Count</td></tr>
<tr><td>64 bytes</td><td>Float4x4</td><td>Global Inverse Transform Matrix</td></tr>
</table>

## Material Format
### Per Material
<table>
  <tr><td>Size</td><td>Type</td><td>Contains</td></tr>
  <tr><td>2 Bytes</td><td>(Bitflags)</td><td>Settings (see table)</td></tr>
  <tr><td>2</td><td>Unsigned Int16</td><td>If enabled via the flag (0x1), this contains the texture index for the diffuse map</td></tr>
  <tr><td>2</td><td>Unsigned Int16</td><td>If enabled via the flag (0x2), this contains the texture index for the normal map</td></tr>
  <tr><td>2</td><td>Unsigned Int16</td><td>If enabled via the flag (0x4), this contains the texture index for the specular map</td></tr>
  <tr><td>4</td><td>4 Bytes BGRA</td><td>Diffuse color of the current material in BGRA</td></tr>
</table>

#### Material Setting Flags
<table>
  <tr><td>& Flag</td><td>Setting</td><td>Flag</td></tr>
  <tr><td>0x01</td><td>Diffuse</td><td>Texture</td></tr>
  <tr><td>0x02</td><td>Normal</td><td>Texture</td></tr>
  <tr><td>0x04</td><td>Specular</td><td>Texture</td></tr>
</table>

## Vertex Format
### Per Mesh
<table>
  <tr><td>2 Bytes</td><td>Bitflags</td><td>Other Settings (see table)</td></tr>
  <tr><td>2 Bytes</td><td>Unsigned</td><td>Material ID</td></tr>
  <tr><td>2 Bytes</td><td>Unsigned</td><td>LOD level - 0 is highest resolution and each successive level is half the size (in each axis). Not all models have LOD's and they are intended to all be drawn together (a model may have multiple level 0 meshes but fewer level 1 and 2 for example)</td></tr>
  <tr><td>2 Bytes</td><td>Unsigned</td><td>How many vertices</td></tr>
  <tr><td>2 Bytes</td><td>Unsigned</td><td>How many elements (x3 for triangle indices)</td></tr>
</table>

#### Mesh Setting Flags
<table>
  <tr><td>& Flag</td><td>Setting Flag</td></tr>
  <tr><td>0x01</td><td>Has Normals</td></tr>
  <tr><td>0x02</td><td>Has UV's</td></tr>
  <tr><td>0x04</td><td>Has Tangents</td></tr>
  <tr><td>0x08</td><td>Has Skin Weights</td></tr>
</table>

### Per Vertex
<table>
<tr><td>Size</td><td>Type</td><td>Value</td></tr>
<tr><td>4 bytes</td><td>Float</td><td>Position X</td></tr>
<tr><td>4 bytes</td><td>Float</td><td>Position Y</td></tr>
<tr><td>4 bytes</td><td>Float</td><td>Position Z</td></tr>

<tr><td>(if Mesh Flag 0x1) 4 bytes</td><td>Float</td><td>Normal X</td></tr>
<tr><td>(if Mesh Flag 0x1) 4 bytes</td><td>Float</td><td>Normal Y</td></tr>
<tr><td>(if Mesh Flag 0x1) 4 bytes</td><td>Float</td><td>Normal Z</td></tr>

<tr><td>(if Mesh Flag 0x2) 4 bytes</td><td>Float</td><td>UV U</td></tr>
<tr><td>(if Mesh Flag 0x2) 4 bytes</td><td>Float</td><td>UV V</td></tr>

<tr><td>(if Mesh Flag 0x4) 4 bytes</td><td>Float</td><td>Tangent X</td></tr>
<tr><td>(if Mesh Flag 0x4) 4 bytes</td><td>Float</td><td>Tangent Y</td></tr>
<tr><td>(if Mesh Flag 0x4) 4 bytes</td><td>Float</td><td>Tangent Z</td></tr>

<tr><td>(if Mesh Flag 0x8) 1 byte</td><td>Uint8_t</td><td>Bone 1 ID</td></tr>
<tr><td>(if Mesh Flag 0x8) 1 byte</td><td>Uint8_t</td><td>Bone 1 Weight</td></tr>
<tr><td>(if Mesh Flag 0x8) 1 byte</td><td>Uint8_t</td><td>Bone 2 ID</td></tr>
<tr><td>(if Mesh Flag 0x8) 1 byte</td><td>Uint8_t</td><td>Bone 2 Weight</td></tr>
<tr><td>(if Mesh Flag 0x8) 1 byte</td><td>Uint8_t</td><td>Bone 3 ID</td></tr>
<tr><td>(if Mesh Flag 0x8) 1 byte</td><td>Uint8_t</td><td>Bone 3 Weight</td></tr>
<tr><td>(if Mesh Flag 0x8) 1 byte</td><td>Uint8_t</td><td>Bone 4 ID</td></tr>
<tr><td>(if Mesh Flag 0x8) 1 byte</td><td>Uint8_t</td><td>Bone 4 Weight</td></tr>
</table>

### Per Index
<table>
<tr><td>Size</td><td>Type</td><td>Value</td></tr>
<tr><td>2 Bytes</td><td>Unsigned Short</td><td>Vertex ID</td></tr>
</table>

## Per Bone (if Model Flag 0x1)
<table>
<tr><td>Size</td><td>Type</td><td>Value</td></tr>
<tr><td>64 bytes</td><td>String</td><td>Bone Name</td></tr>
<tr><td>1 byte</td><td>Uint8_t<float></td><td>Parent Bone Id</td></tr>
<tr><td>64 bytes</td><td>Float4x4</td><td>Bone Offset Matrix</td></tr>
</table>

## Animation Format (if Model Flag 0x1)

### Per Animation
<table>
<tr><td>Size</td><td>Type</td><td>Value</td></tr>
<tr><td>64 bytes</td><td>String</td><td>Bone Name</td></tr>
<tr><td>4 bytes</td><td>Float</td><td>Duration</td></tr>
<tr><td>4 bytes</td><td>Float</td><td>Playback Rate</td></tr>
<tr><td>1 byte</td><td>Uint8_t</td><td>Channel Count (T/R/S)</td></tr>
</table>

### Per Animation Channel
<table>
<tr><td>Size</td><td>Type</td><td>Value</td></tr>
<tr><td>1 byte</td><td>Uint8_t</td><td>Bone Id</td></tr>
<tr><td>1 byte</td><td>Uint8_t</td><td>Parent Channel Id</td></tr>
<tr><td>2 bytes</td><td>Unsigned Short</td><td>Translate Key Count</td></tr>
<tr><td>4 bytes</td><td>Float</td><td>Time</td></tr>
<tr><td>4 bytes</td><td>Float</td><td>Translate X</td></tr>
<tr><td>4 bytes</td><td>Float</td><td>Translate Y</td></tr>
<tr><td>4 bytes</td><td>Float</td><td>Translate Z</td></tr>
<tr><td>2 bytes</td><td>Unsigned Short</td><td>Rotate Key Count</td></tr>
<tr><td>4 bytes</td><td>Float</td><td>Time</td></tr>
<tr><td>4 bytes</td><td>Float</td><td>Rotate X</td></tr>
<tr><td>4 bytes</td><td>Float</td><td>Rotate Y</td></tr>
<tr><td>4 bytes</td><td>Float</td><td>Rotate Z</td></tr>
<tr><td>2 bytes</td><td>Unsigned Short</td><td>Scale Key Count</td></tr>
<tr><td>4 bytes</td><td>Float</td><td>Time</td></tr>
<tr><td>4 bytes</td><td>Float</td><td>Scale X</td></tr>
<tr><td>4 bytes</td><td>Float</td><td>Scale Y</td></tr>
<tr><td>4 bytes</td><td>Float</td><td>Scale Z</td></tr>
</table>
