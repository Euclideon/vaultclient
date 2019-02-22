import array
from struct import *
import sys

version = 1
twobytemax = 0xFFFF
filemax = 1000000000

importFilename = "BCC_Bus.obj"
outputFilename = "Test"

textures = {}

materialList = {'__white' : 0} # Default white for 'usemtl'
materials = [[0x0, 0xFF, 0xFF, 0xFF, 0xFF]] # flags, BGRA
defautMatUsed = False

meshList = {'__baseMesh' : 0} # right now material list indexes are used to reference meshes, procVerts, faces
meshProperties = [[False, False, False, 0x0, 0x0, 0]] #flags (normals, UVs, tangents), matID, LOD
currMesh = [0]

min = [] # X, Y, Z
max = []

def buildFlags(inputBools):
  x = 0
  y = 1

  for i in range(len(inputBools)):
    if inputBools[i]:
      x += y
    y *= 2

  return x

def addMaterial(matName, flags, blue, green, red):
  materialList[matName] = len(materials)
  materials.append([flags, int(blue * 0xFF), int(green * 0xFF), int(red * 0xFF), 0xFF])

def addMesh():
  meshProperties.append([False, False, False, len(meshList), 0x0])
  faces.append([])
  procVerts.append([])
  procIndices.append([])

def readMaterials(filename):
  matFile = open(filename)
  matLines = matFile.readlines()
  matFile.close()

  matName = None
  flags = 0 # No texture indexes output

  for line in matLines:
    word = line.strip().split(" ")
    if word[0] == "newmtl":
      if matName is not None:
        addMaterial(matName, flags, blue, green, red)
      matName = ' '.join(word[1:])
    if word[-1] not in list(textures.keys()):
      if word[0] == "map_Ka":
        textures[word[-1]] = len(textures)
      if word[0] == "map_Kd":
        textures[word[-1]] = len(textures)
      if word[0] == "map_Ks":
        textures[word[-1]] = len(textures)
    if word[0] == "Kd":
      if word[1] != "xyz" and word[1] != "spectral":
        red = float(word[1])
        blue = float(word[2]) if len(word) > 2 else red
        green = float(word[3]) if len(word) > 3 else blue
  addMaterial(matName, flags, blue, green, red)

if len(sys.argv) > 1:
  importFilename = sys.argv[1]
frh = open(importFilename)
lines = frh.readlines()
frh.close()

verts = []
normals = []
uvs = []

procVerts = [[]]

faces = [[]]

vertIndexes = {}
procIndices = [[]]

currMat = 0
LOD = 0

for line in lines:
  tokens = line.strip().split(" ")

  if tokens[0] == "v":
    if tokens[1] == "":
      verts.append(tokens[2:])
    else:
      verts.append(tokens[1:])

  elif tokens[0] == "vn":
    if tokens[1] == "":
      normals.append(tokens[2:])
    else:
      normals.append(tokens[1:])

  # UV U X
  elif tokens[0] == "vt":
    if tokens[1] == "":
      uvs.append(tokens[2:4])
    else:
      uvs.append(tokens[1:3])

  elif tokens[0] == "f":
    for i in currMesh:
      meshProperties[i][3] = currMat
      if len(tokens) == 5: #quads
        faces[i].append([tokens[1], tokens[2], tokens[3]])
        faces[i].append([tokens[3], tokens[4], tokens[1]])
      if len(tokens) == 4: #tris
        faces[i].append([tokens[1], tokens[2], tokens[3]])

  elif tokens[0] == "mtllib": #materials file
    readMaterials(' '.join(tokens[1:]))

  elif tokens[0] == "usemtl":
    if len(tokens) > 1:
      currMat = materialList[' '.join(tokens[1:])]
    else: # Default white
      currMat = 0
      defautMatUsed = True

  elif tokens[0] == "lod":
    for i in currMesh:
      meshProperties[i][4] = tokens[1]

  elif tokens[0] == "g":
    if len(tokens) > 1:
      currMesh = []
      for i in range(len(tokens) - 1):
        group = tokens[i+1]
        if group not in meshList:
          addMesh()
          meshList[group] = len(meshList)
        currMesh.append(meshList[group])

for meshIndex in range(len(faces)):
  for face in faces[meshIndex]:
    for trip in face:
      vbs = trip.split('/')

      key = vbs[0]+"_"+vbs[1]+"_"+vbs[2]
      iv = int(vbs[0]) - 1
      it = int(vbs[1]) - 1
      io = int(vbs[2]) - 1

      if io > 0:
        meshProperties[meshIndex][0] = True
      if it > 0:
        meshProperties[meshIndex][1] = True

      value = [ float(verts[iv][0]), float(verts[iv][1]), float(verts[iv][2]), float(normals[io][0]), float(normals[io][1]), float(normals[io][2]), float(uvs[it][0]), float(uvs[it][1]) ]

      if key not in vertIndexes:
        vertIndexes[key] = len(procVerts[meshIndex])
        procVerts[meshIndex].append(value)

      procIndices[meshIndex].append(vertIndexes[key])

# Get min/max values from vertices
# Initialise to first vert
min = procVerts[len(procVerts) - 1][0][0:3]
max = procVerts[len(procVerts) - 1][0][0:3]
for i in procVerts:
  for vert in i:
    for x in range(3):
      if min[x] > vert[x]:
        min[x] = vert[x]
      if max[x] < vert[x]:
        max[x] = vert[x]

if len(sys.argv) == 3:
  outputFilename = sys.argv[2]
fh = open(outputFilename + ".vsm", 'wb')

if len(materials) > twobytemax:
    print("Error: Too many materials, maximum is ")
    print(twobytemax)
if len(meshProperties) > twobytemax:
    print("Error: Too many materials, maximum is ")
    print(twobytemax)

validMeshes = 0
for i in range(len(meshList)):
  if len(procVerts[i]) > 1:
    validMeshes += 1

# Header
fh.write(pack('4s6H6f', b"VSMF", version, 0, len(materials) - 1 + defautMatUsed, validMeshes, len(textures), 0, min[0], min[1], min[2], max[0], max[1], max[2] ))

# Materials
for mat in range(len(materials)):
  if mat == 0 and defautMatUsed == False:
    continue
  fh.write(pack('H4B', materials[mat][0], materials[mat][4], materials[mat][3], materials[mat][2], materials[mat][1]))

# Mesh
for i in range(len(meshList)):
  if len(procVerts[i]) < 1:
    continue

  # Build flags
  x = buildFlags(meshProperties[i][0:3])

  if len(procVerts[i]) > twobytemax:
    print("Error: Too many vertexes, maximum is ")
    print(twobytemax)

  fh.write(pack('5H', x, meshProperties[i][3], meshProperties[i][4], len(procVerts[i]), len(procIndices[i])))

  for j in range(len(procVerts[i])):
    fh.write(pack('3f', procVerts[i][j][0], procVerts[i][j][1], procVerts[i][j][2]))
    if meshProperties[i][0]:
      fh.write(pack('3f', procVerts[i][j][3], procVerts[i][j][4], procVerts[i][j][5]))
    if meshProperties[i][1]:
      fh.write(pack('2f', procVerts[i][j][6], procVerts[i][j][7]))
    if meshProperties[i][2]:
      fh.write(pack('3f', procVerts[i][j][8], procVerts[i][j][9], procVerts[i][j][10]))

  for j in range(len(procIndices[i])):
    fh.write(pack('H', procIndices[i][j]))

for i in range(len(textures)):
  textureFile = open(list(textures.keys())[i], 'rb')
  textureData = textureFile.read(filemax)

  fh.write(pack('I', len(textureData)))
  fh.write(textureData)

fh.close()
