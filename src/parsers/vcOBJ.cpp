#define _CRT_SECURE_NO_WARNINGS
#include "vcOBJ.h"
#include "udFile.h"
#include "udStringUtil.h"

#include "vdkTriangleVoxelizer.h"

#define TEXT_BUFFER_SIZE            (1048576) // 1MB buffer
#define TEXT_BUFFER_REPLENISH_MARK  (65536)   // Move unread data back and re-fill buffer whenever the last 64k is reached

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, February 2019
udResult vcOBJ_LoadMtlLib(vcOBJ *pOBJ, const char *pFilename)
{
  udResult result;
  char *pText = nullptr;
  vcOBJ::Material *pMat = nullptr;
  int64_t length;
  int charCount;

  UD_ERROR_CHECK(udFile_Load(pFilename, (void **)& pText, &length));
  for (const char *p = udStrSkipWhiteSpace(pText); *p; p = udStrSkipWhiteSpace(p))
  {
    if (udStrBeginsWithi(p, "newmtl"))
    {
      // Create a new material and initialise with defaults
      UD_ERROR_CHECK(pOBJ->materials.PushBack(&pMat));
      memset(pMat, 0, sizeof(*pMat)); // Zeros to start with
      p += 6;
      while (*p == ' ' || *p == '\t')
        ++p;
      for (charCount = 0; p[charCount] && p[charCount] != '\r' && p[charCount] != '\n';)
        ++charCount;
      udStrncpy(pMat->name, p, charCount);
      pMat->Ka = pMat->Kd = udFloat3::one();
      pMat->Ks = udFloat3::zero();
      pMat->d = 1.0f; // Default to not transparent
      pMat->illum = 1; // Not sure there is a default, quite possibly always specified, but 1 is most compatible with UD
      pMat->blendU = pMat->blendV = true;
    }
    else if (udStrBeginsWithi(p, "Ka ") || udStrBeginsWithi(p, "Kd ") || udStrBeginsWithi(p, "Ks "))
    {
      UD_ERROR_NULL(pMat, udR_ParseError);
      udFloat3 v;
      int channel = p[1] | ('a' - 'A'); // Get tolower'd channel before advancing p

      p = udStrSkipWhiteSpace(p + 3);
      int fields = sscanf(p, "%f %f %f", &v.x, &v.y, &v.z);
      UD_ERROR_IF(!fields, udR_ParseError);
      if (fields == 1)
        v.z = v.y = v.x;
      else if (fields == 2)
        v.z = v.y;

      switch (channel)
      {
      case 'a': pMat->Ka = v; break;
      case 'd': pMat->Kd = v; break;
      case 's': pMat->Ks = v; break;
      default:
        break; // Allow (and ignore) other kinds of channels to be declared
      }
    }
    else if (udStrBeginsWithi(p, "map_"))
    {
      char *pChannel = nullptr;
      if (udStrBeginsWithi(p, "map_Ka "))
        pChannel = pMat->map_Ka;
      else if (udStrBeginsWithi(p, "map_Kd "))
        pChannel = pMat->map_Kd;
      else if (udStrBeginsWithi(p, "map_Ks "))
        pChannel = pMat->map_Ks;
      else if (udStrBeginsWithi(p, "map_Ns "))
        pChannel = pMat->map_Ns;
      else if (udStrBeginsWithi(p, "map_Ao "))
        pChannel = pMat->map_Ao;
      else
        pChannel = nullptr; // Allow (and ignore) other kinds of channels to be declared

      if (pChannel && (p[7] != '\r' && p[7] != '\n'))
      {
        p = udStrSkipWhiteSpace(p + 7);
        for (charCount = 0; p[charCount] && p[charCount] != '\r' && p[charCount] != '\n';)
          ++charCount;
        // Trim any trailing space from the filename
        while (charCount > 0 && p[charCount - 1] == ' ')
          --charCount;

        udStrncpy(pChannel, udLengthOf(pMat->map_Kd), p, charCount);
      }
    }
    else if (udStrBeginsWithi(p, "d "))
    {
      UD_ERROR_NULL(pMat, udR_ParseError);
      p = udStrSkipWhiteSpace(p + 2);
      if (udStrBeginsWithi(p, "-halo ")) // Ignore halo option
        p += 6;
      pMat->d = udStrAtof(p);
    }
    else if (udStrBeginsWithi(p, "Tr "))
    {
      UD_ERROR_NULL(pMat, udR_ParseError);
      p = udStrSkipWhiteSpace(p + 3);
      if (udStrBeginsWithi(p, "-halo ")) // Ignore halo option
        p += 6;
      pMat->d = 1.0f - udStrAtof(p);
    }
    else if (udStrBeginsWithi(p, "illum "))
    {
      UD_ERROR_NULL(pMat, udR_ParseError);
      p = udStrSkipWhiteSpace(p + 6);
      pMat->illum = udStrAtoi(p);
    }
    else if (udStrBeginsWithi(p, "blendu "))
    {
      UD_ERROR_NULL(pMat, udR_ParseError);
      p = udStrSkipWhiteSpace(p + 7);
      pMat->blendU = udStrBeginsWithi(p, "on");
    }
    else if (udStrBeginsWithi(p, "blendv "))
    {
      UD_ERROR_NULL(pMat, udR_ParseError);
      p = udStrSkipWhiteSpace(p + 7);
      pMat->blendV = udStrBeginsWithi(p, "on");
    }
    else if (udStrBeginsWithi(p, "clamp "))
    {
      UD_ERROR_NULL(pMat, udR_ParseError);
      p = udStrSkipWhiteSpace(p + 6);
      pMat->clamp = udStrBeginsWithi(p, "on");
    }
    // Just ignore unrecognised lines
    p = udStrSkipToEOL(p);
  }

epilogue:
  udFree(pText);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, February 2019
udResult vcOBJ_Load(vcOBJ **ppOBJ, const char *pFilename)
{
  udResult result;
  vcOBJ *pOBJ = nullptr;
  udFile *pFile = nullptr;
  int64_t filePos = 0;
  int64_t fileLength = 0;
  char *pTextBuffer = nullptr;
  size_t textBufLen = TEXT_BUFFER_SIZE; // Primer values to trigger a replenish in first loop
  size_t bufferPos = TEXT_BUFFER_SIZE;
  int lineNumber = 1, charCount, matIndex = -1;

  UD_ERROR_NULL(ppOBJ, udR_InvalidParameter_);
  UD_ERROR_NULL(pFilename, udR_InvalidParameter_);
  pOBJ = udAllocType(vcOBJ, 1, udAF_Zero);
  UD_ERROR_NULL(pOBJ, udR_MemoryAllocationFailure);
  pTextBuffer = udAllocType(char, TEXT_BUFFER_SIZE + 1, udAF_None);
  UD_ERROR_NULL(pTextBuffer, udR_MemoryAllocationFailure);
  pTextBuffer[TEXT_BUFFER_SIZE] = 0; // Trailing nul terminator to simplify nul terminating the last line

  pOBJ->basePath.SetFromFullPath(pFilename);
  pOBJ->positions.Init(65536);
  pOBJ->colors.Init(65536);
  pOBJ->uvs.Init(65536);
  pOBJ->normals.Init(65536);
  pOBJ->faces.Init(65536);
  pOBJ->materials.Init(4);

  UD_ERROR_CHECK(udFile_Open(&pFile, pFilename, udFOF_Read, &fileLength));

  // Read the entire file and decode the text
  do
  {
    // Read next chunk of buffer if necessary
    if (bufferPos >= (TEXT_BUFFER_SIZE - TEXT_BUFFER_REPLENISH_MARK) && textBufLen == TEXT_BUFFER_SIZE)
    {
      size_t actualRead;
      textBufLen -= bufferPos;
      memmove(pTextBuffer, pTextBuffer + bufferPos, textBufLen);
      bufferPos = 0;
      UD_ERROR_CHECK(udFile_Read(pFile, pTextBuffer + textBufLen, (size_t)TEXT_BUFFER_SIZE - textBufLen, filePos, udFSW_SeekSet, &actualRead, &filePos));
      textBufLen += actualRead;
      pTextBuffer[textBufLen] = 0; // Ensure nul terminated, this is safe because we allocate 1 extra byte
    }

    udStrSkipWhiteSpace(pTextBuffer + bufferPos, &charCount, &lineNumber);
    bufferPos += charCount;
    if (pTextBuffer[bufferPos] == '\0')
    {
      break;
    }
    else if (pTextBuffer[bufferPos] == '#')
    {
      ++bufferPos;
      int prevLine = lineNumber;
      udStrSkipWhiteSpace(pTextBuffer + bufferPos, &charCount, &lineNumber);

      if (lineNumber == prevLine) // Comment terminated already if line number has changed
      {
        bufferPos += charCount;
        if (udStrBeginsWithi(pTextBuffer + bufferPos, "Scene Origin:") || udStrBeginsWithi(pTextBuffer + bufferPos, "WGS84 Origin:"))
        {
          int offset = 13;
          // Detect geo-location info contained within comments
          udDouble3 v;
          v.x = udStrAtof64(pTextBuffer + bufferPos + offset, &charCount);
          offset += charCount;
          v.y = udStrAtof64(pTextBuffer + bufferPos + offset, &charCount);
          offset += charCount;
          v.z = udStrAtof64(pTextBuffer + bufferPos + offset, &charCount);
          offset += charCount;
          if (udStrBeginsWithi(pTextBuffer + bufferPos, "Scene"))
            pOBJ->sceneOrigin = v;
          else
            pOBJ->wgs84Origin = v;
        }
      }
    }
    else if (unsigned(pTextBuffer[bufferPos]) > 127) // cast required as some platforms have signed chars, some unsigned chars
    {
      // Non-ascii at beginning of line indicates this is not an OBJ file
      UD_ERROR_SET(udR_ParseError);
    }
    else if (pTextBuffer[bufferPos + 1] == ' ')
    {
      // Single-letter commands in here
      if (pTextBuffer[bufferPos] == 'v')
      {
        // Vertex
        double values[6];
        size_t valueCount;
        bufferPos += 2;
        for (valueCount = 0; valueCount < udLengthOf(values); ++valueCount)
        {
          values[valueCount] = udStrAtof64(pTextBuffer + bufferPos, &charCount);
          if (!charCount)
            break;
          bufferPos += charCount;
        }
        UD_ERROR_IF(valueCount != 3 && valueCount != 4 && valueCount != 6, udR_ParseError);

        // If color specified or has ever been specified, make sure color array is padded to match positions array
        while ((valueCount == 6 || pOBJ->colors.length > 0) && pOBJ->colors.length < pOBJ->positions.length)
          UD_ERROR_CHECK(pOBJ->colors.PushBack(udFloat3::one()));

        // Append the position
        UD_ERROR_CHECK(pOBJ->positions.PushBack(udDouble3::create(values[0], values[1], values[2])));
        // Append the color, or white if color has been previously specified but not on this vertex
        if (valueCount == 6)
          UD_ERROR_CHECK(pOBJ->colors.PushBack(udFloat3::create((float)values[3], (float)values[4], (float)values[5])));
        else if (pOBJ->colors.length > 0)
          UD_ERROR_CHECK(pOBJ->colors.PushBack(udFloat3::one()));
      }
      else if (pTextBuffer[bufferPos] == 'f')
      {
        vcOBJ::Face *pFace = nullptr;
        int vertCount;
        UD_ERROR_CHECK(pOBJ->faces.PushBack(&pFace));
        bufferPos += 2;

        for (vertCount = 0; ; ++vertCount)
        {
          // Position index
          int pos = udStrAtoi(pTextBuffer + bufferPos, &charCount);
          if (!charCount)
            break;
          bufferPos += charCount;

          int vert = vertCount;
          if (vertCount > 2)
          {
            // For 4th and onwards verts, make new faces assuming the list of verts was a strip
            vcOBJ::Face *pPrevFace = pFace;
            UD_ERROR_CHECK(pOBJ->faces.PushBack(&pFace));
            pFace->verts[0] = pPrevFace->verts[0];
            pFace->verts[1] = pPrevFace->verts[2];
            vert = 2;
          }

          pFace->verts[vert].pos = (pos >= 0) ? pos - 1 : pos + (int)pOBJ->positions.length; // Handle relative indices
          pFace->verts[vert].uv = pFace->verts[vert].nrm = -1; // Defaults

          if (pTextBuffer[bufferPos] == '/')
          {
            ++bufferPos;
            int uvi = udStrAtoi(pTextBuffer + bufferPos, &charCount);
            pFace->verts[vert].uv = (uvi >= 0) ? uvi - 1 : uvi + (int)pOBJ->uvs.length; // Handle relative indices
            bufferPos += charCount;
            if (pTextBuffer[bufferPos] == '/')
            {
              ++bufferPos;
              // Handle the case where no normal is specified, typically a space, but handle a newline also
              if (pTextBuffer[bufferPos] != ' ' && pTextBuffer[bufferPos] != '\r' && pTextBuffer[bufferPos] != '\n')
              {
                int nmi = udStrAtoi(pTextBuffer + bufferPos, &charCount);
                pFace->verts[vert].nrm = (nmi >= 0) ? nmi - 1 : nmi + (int)pOBJ->normals.length; // Handle relative indices
                bufferPos += charCount;
              }
            }
          }
          pFace->mat = matIndex;

          while (pTextBuffer[bufferPos] == ' ')
            ++bufferPos;
        }
        UD_ERROR_IF(vertCount < 3, udR_ParseError);
      }
      else if (pTextBuffer[bufferPos] == 'l')
      {
        // Line - ignored for the moment
      }
      else if (pTextBuffer[bufferPos] == 'o')
      {
        // Object name ignored
      }
      else if (pTextBuffer[bufferPos] == 'g')
      {
        // Group name ignored
      }
      else if (pTextBuffer[bufferPos] == 's')
      {
        // Smooth shading 1/0/on/off ignored
      }
      else
      {
        UD_ERROR_SET(udR_ParseError);
      }
    }
    else if (pTextBuffer[bufferPos + 2] == ' ')
    {
      // Two-letter commands here
      if (pTextBuffer[bufferPos] == 'v' && pTextBuffer[bufferPos + 1] == 't')
      {
        // UV
        udFloat3 uv;

        bufferPos += 2;
        uv.x = udStrAtof(pTextBuffer + bufferPos, &charCount);
        bufferPos += charCount;
        UD_ERROR_IF(!charCount, udR_ParseError);

        uv.y = udStrAtof(pTextBuffer + bufferPos, &charCount);
        bufferPos += charCount;
        UD_ERROR_IF(!charCount, udR_ParseError);

        // Third component of normal is optional
        uv.z = udStrAtof(pTextBuffer + bufferPos, &charCount);
        bufferPos += charCount;

        UD_ERROR_CHECK(pOBJ->uvs.PushBack(uv));
      }
      else if (pTextBuffer[bufferPos] == 'v' && pTextBuffer[bufferPos + 1] == 'n')
      {
        // Normal
        double values[3];
        size_t valueCount;
        bufferPos += 2;
        for (valueCount = 0; valueCount < udLengthOf(values); ++valueCount)
        {
          values[valueCount] = udStrAtof64(pTextBuffer + bufferPos, &charCount);
          if (!charCount)
            break;
          bufferPos += charCount;
        }
        UD_ERROR_IF(valueCount != 3, udR_ParseError);

        // Append the normal
        UD_ERROR_CHECK(pOBJ->normals.PushBack(udFloat3::create((float)values[0], (float)values[1], (float)values[2])));
      }
      else if (pTextBuffer[bufferPos] == 'v' && pTextBuffer[bufferPos + 1] == 'p')
      {
        // Parameter - ignored for the moment
      }
      else
      {
        UD_ERROR_SET(udR_ParseError);
      }
    }
    else if (udStrBeginsWithi(pTextBuffer + bufferPos, "usemtl "))
    {
      // Material
      bufferPos += 7;
      for (charCount = 0; pTextBuffer[bufferPos + charCount] && pTextBuffer[bufferPos + charCount] != '\r' && pTextBuffer[bufferPos + charCount] != '\n';)
        ++charCount;
      for (matIndex = ((int)pOBJ->materials.length) - 1; matIndex >= 0; --matIndex)
      {
        if (udStrncmpi(pOBJ->materials[matIndex].name, pTextBuffer + bufferPos, charCount) == 0 && udStrlen(pOBJ->materials[matIndex].name) == (size_t)charCount)
          break;
      }
    }
    else if (udStrBeginsWithi(pTextBuffer + bufferPos, "mtllib "))
    {
      // Material library import
      bufferPos += 7;
      udStrSkipWhiteSpace(pTextBuffer + bufferPos, &charCount, &lineNumber);
      bufferPos += charCount;
      for (charCount = 0; pTextBuffer[bufferPos + charCount] && pTextBuffer[bufferPos + charCount] != '\r' && pTextBuffer[bufferPos + charCount] != '\n';)
        ++charCount;
      char temp = pTextBuffer[bufferPos + charCount];
      pTextBuffer[bufferPos + charCount] = 0;
      udFilename f(pTextBuffer + bufferPos);
      pTextBuffer[bufferPos + charCount] = temp;
      pOBJ->basePath.SetFilenameWithExt(f.GetFilenameWithExt());

      //Allow to load an object file without a material file.
      result = vcOBJ_LoadMtlLib(pOBJ, pOBJ->basePath.GetPath());
      if (result != udR_OpenFailure || result != udR_Success)
        UD_ERROR_SET(result);
    }
    else
    {
      UD_ERROR_SET(udR_ParseError);
    }

    // If not already at EOL, skip to it
    if (pTextBuffer[bufferPos] != '\r' && pTextBuffer[bufferPos] != '\n')
    {
      udStrSkipToEOL(pTextBuffer + bufferPos, &charCount, &lineNumber);
      bufferPos += charCount;
    }
  } while (pTextBuffer[bufferPos] && bufferPos != textBufLen);
  UD_ERROR_IF(pOBJ->positions.length == 0, udR_ParseError);

  pOBJ->basePath.SetFilenameWithExt("");

  *ppOBJ = pOBJ;
  pOBJ = nullptr;
  result = udR_Success;

epilogue:
  udFile_Close(&pFile);
  udFree(pTextBuffer);
  if (pOBJ)
    vcOBJ_Destroy(&pOBJ);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, February 2019
void vcOBJ_Destroy(vcOBJ **ppOBJ)
{
  if (ppOBJ && *ppOBJ)
  {
    vcOBJ *pOBJ = *ppOBJ;

    pOBJ->positions.Deinit();
    pOBJ->colors.Deinit();
    pOBJ->uvs.Deinit();
    pOBJ->normals.Deinit();
    pOBJ->faces.Deinit();
    pOBJ->materials.Deinit();
    pOBJ = nullptr;
    udFree(*ppOBJ);
  }
}
