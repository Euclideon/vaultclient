import array

importFilename = "Model.obj"

frh = open(importFilename)
lines = frh.readlines()
frh.close()

verts = []
normals = []
uvs = []

procVerts = []
procIndices = []

tris = []

vertIndexes = {}

for x in lines:
  bits = x.strip().split(" ")

  if bits[0] == "v":
    if bits[1] == "":
      verts.append(bits[2:])
    else:
      verts.append(bits[1:])
  elif bits[0] == "vn":
    if bits[1] == "":
      normals.append(bits[2:])
    else:
      normals.append(bits[1:])
  elif bits[0] == "vt":
    if bits[1] == "":
      uvs.append(bits[2:])
    else:
      uvs.append(bits[1:])
  elif bits[0] == "f":
    if len(bits) == 5: #quads
      tris.append([bits[1], bits[2], bits[3]])
      tris.append([bits[3], bits[4], bits[1]])
    if len(bits) == 4: #tris
      tris.append([bits[1], bits[2], bits[3]])

for tri in tris:
  for vert in tri:
    vbs = vert.split('/')
    key = "?"
    value = [0,0,0]
    if len(vbs) == 1:
      key = vbs[0]
      iv = int(vbs[0]) - 1
      value = [ float(verts[iv][0]), float(verts[iv][1]), float(verts[iv][2]) ]
    elif len(vbs) >= 3:
      key = vbs[0]+"_"+"_"+vbs[1]+"_"+vbs[2]
      iv = int(vbs[0]) - 1
      it = int(vbs[1]) - 1
      io = int(vbs[2]) - 1
      value = [ float(verts[iv][0]), float(verts[iv][1]), float(verts[iv][2]), float(normals[io][0]), float(normals[io][1]), float(normals[io][2]), float(uvs[it][0]), float(uvs[it][1]) ]

    if not vertIndexes.has_key(key):
      vertIndexes[key] = len(procVerts)
      procVerts.append(value)

    procIndices.append(vertIndexes[key])

fh = open(importFilename + ".vsm", 'wb')

array.array('H', [len(procVerts), len(procIndices)]).tofile(fh)

print [len(procVerts), len(procIndices)]

for x in procVerts:
  array.array('f', x).tofile(fh)

array.array('H', procIndices).tofile(fh)

fh.close()