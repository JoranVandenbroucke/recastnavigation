//
// Copyright (c) 2024.
// Author: Joran.
//

#pragma once
#include "Generators.h"

#include <Recast.h>
#include <RecastAlloc.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <fstream>
#include <span>
#include <sstream>
#include <string>
#include <vector>

// For comparing to rcVector in benchmarks.
const float g_cellHeight = 0.2f;
const float g_agentRadius = 0.0f;
const float g_agentHeight = 2.0f;
const float g_agentMaxClimb = 0.9f;
const float g_agentMaxSlope = 45.0f;
const float g_edgeMaxLen = 12.0f;
const float g_regionMinSize = 0.0f;
const float g_regionMergeSize = 0.0f;
const float g_edgeMaxError = 1.3f;
const float g_vertsPerPoly = 6.0f;
const float g_detailSampleDist = 6.0f;
const float g_detailSampleMaxError = 1.0f;
const int g_loopCount = 100;
const bool g_filterLedgeSpans = true;
const bool g_filterWalkableLowHeightSpans = true;
const bool g_filterLowHangingObstacles = true;

const char header[] =
    "ID,"
    "Method,"
    "Environment,"
    "Grid Size,"
    "Environment Size,"
    "Cell Count,"
    "Total (ms),"
    "Temp (ms),"
    "Rasterize Triangles (ms),"
    "Build Compact Height Field (ms),"
    "Build Contours (ms),"
    "Build Contours Trace (ms),"
    "Build Contours Simplify (ms),"
    "Filter Border (ms),"
    "Filter Walkable (ms),"
    "Median Area (ms),"
    "Filter Low Obstacles (ms),"
    "Build Polymesh (ms),"
    "Merge Polymeshes (ms),"
    "Erode Area (ms),"
    "Mark Box Area (ms),"
    "Mark Cylinder Area (ms),"
    "Mark Convex Area (ms),"
    "Build Distance Field (ms),"
    "Build Distance Field Distance (ms),"
    "Build Distance Field Blur (ms),"
    "Build Regions (ms),"
    "Build Regions Watershed (ms),"
    "Build Regions Expand (ms),"
    "Build Regions Flood (ms),"
    "Build Regions Filter (ms),"
    "Build Layers (ms),"
    "Build Polymesh Detail (ms),"
    "Merge Polymesh Details (ms)";

struct Vertex {
  int x;
  int y;
};

struct Edge {
  Vertex v1{};
  Vertex v2{};
};

inline std::array<float, g_loopCount * RC_MAX_TIMERS> generateThesisTimes(BuildContext &context, const InputGeom &pGeom, rcConfig &config, int *&pEdges, int &edgeCount) {
  std::array<float, g_loopCount * RC_MAX_TIMERS> times{};
  for (int i{}; i < g_loopCount; i++) {
    rcPolyMesh *pMesh{nullptr};
    rcPolyMeshDetail *pDMesh{nullptr};
    if (!generateTheses(context, pGeom, config, g_filterLowHangingObstacles, g_filterLedgeSpans, g_filterWalkableLowHeightSpans, pMesh, pDMesh, pEdges, edgeCount))
      context.dumpLog("Error Thesis:");
    rcFreePolyMesh(pMesh);
    rcFreePolyMeshDetail(pDMesh);
    pMesh = nullptr;
    pDMesh = nullptr;
    if (i != g_loopCount - 1) {
      rcFree(pEdges);
    }
    const int offset{i * RC_MAX_TIMERS};
    for (int j = 0; j < RC_MAX_TIMERS; ++j) {
      times[offset + j] = static_cast<float>(context.getAccumulatedTime(static_cast<rcTimerLabel>(j))) * 1e-3f;
    }
  }
  return times;
}

inline std::array<float, g_loopCount * RC_MAX_TIMERS> generateSingleMeshTimes(BuildContext &context, const InputGeom &pGeom, rcConfig &config) {
  std::array<float, g_loopCount * RC_MAX_TIMERS> times{};
  for (int i{}; i < g_loopCount; i++) {
    rcPolyMesh *pMesh{nullptr};
    rcPolyMeshDetail *pDMesh{nullptr};
    generateSingle(context, pGeom, config, g_filterLowHangingObstacles, g_filterLedgeSpans, g_filterWalkableLowHeightSpans, pMesh, pDMesh);
    rcFreePolyMesh(pMesh);
    rcFreePolyMeshDetail(pDMesh);
    pMesh = nullptr;
    pDMesh = nullptr;

    const int offset{i * RC_MAX_TIMERS};
    for (int j = 0; j < RC_MAX_TIMERS; ++j) {
      times[offset + j] = static_cast<float>(context.getAccumulatedTime(static_cast<rcTimerLabel>(j))) * 1e-3f;
    }
  }
  return times;
}

inline void writeCsvFile(const bool isThesis, const std::string &filePath, const std::string &environmentName, const InputGeom &pGeom, rcConfig &config, const float gridSize, const std::array<float, g_loopCount * RC_MAX_TIMERS> &timerData) {
  static int count{};
  try {
    system(("mkdir " + filePath).c_str());
  } catch (std::exception &) {
  }
  const float width = pGeom.getMeshBoundsMax()[0] - pGeom.getMeshBoundsMin()[0];
  const float depth = pGeom.getMeshBoundsMax()[2] - pGeom.getMeshBoundsMin()[2];
  float height = pGeom.getMeshBoundsMax()[1] - pGeom.getMeshBoundsMin()[1];

  if (height < 1e-3f)
    height = 1.f;
  std::ofstream csvFile{filePath + "/Timings.csv", std::ios::out | std::ios::app};
  for (int i{}; i < g_loopCount; ++i) {
    csvFile << count++ << (isThesis ? ",Thesis," : ",Default,") << environmentName << ',' << gridSize << ',';
    csvFile << (int)(width * height * depth) << ',';
    csvFile << config.width * config.height << ',';
    for (int j{}; j < RC_MAX_TIMERS; ++j) {
      csvFile << timerData[i * RC_MAX_TIMERS + j];
      if (j != RC_MAX_TIMERS - 1)
        csvFile << ',';
    }
    csvFile << std::endl;
  }
  csvFile.close();
}

inline void generateTimes(const std::string &output, const std::string &environmentName, const float gridSize, BuildContext &context, const InputGeom &pGeom, rcConfig &config, int *&pEdge, int &edgeCount) {
  const std::array<float, g_loopCount * RC_MAX_TIMERS> defaultTimes{generateSingleMeshTimes(context, pGeom, config)};
  const std::array<float, g_loopCount * RC_MAX_TIMERS> thesisTimes{generateThesisTimes(context, pGeom, config, pEdge, edgeCount)};

  writeCsvFile(false, output, environmentName, pGeom, config, gridSize, defaultTimes);
  writeCsvFile(true, output, environmentName, pGeom, config, gridSize, thesisTimes);
}

inline bool compareEdges(const Edge &edge1, const Edge &edge2) {
  if (edge1.v1.x == edge2.v1.x)
    return edge1.v1.y < edge2.v1.y;
  return edge1.v1.x < edge2.v1.x;
}

inline bool operator<(const Edge &e1, const Edge &e2) { return compareEdges(e1, e2); }

inline std::ofstream &startSvg(std::ofstream &file, const std::string &path, const uint32_t width, const uint32_t height) {
  if (file.is_open())
    return file;
  file.open(path.data());
  if (!file.is_open())
    return file;
  file << "<svg width=\"" << width << "\" height=\"" << height << "\" xmlns=\"http://www.w3.org/2000/svg\">\n";
  return file;
}
inline std::ofstream &writeSvgLine(std::ofstream &file, const std::vector<Edge> &edges, const std::string &color) {
  if (!file.is_open())
    return file;
  std::for_each(edges.cbegin(), edges.cend(), [&file, &color](const Edge &edge) {
    file << "<line x1=\"" << edge.v1.x << "\" y1=\"" << edge.v1.y << "\" x2=\"" << edge.v2.x << "\" y2=\"" << edge.v2.y << "\" style=\"stroke: " << color << "; stroke-width: 2;\" />\n";
  });
  return file;
}
inline std::ofstream &endSvg(std::ofstream &file) {
  if (!file.is_open())
    return file;
  file << R"(</svg>)" << std::endl;
  file.close();
  return file;
}
inline void processBourderEdges(const std::string &input, const std::string &output, const std::string &name, const InputGeom &pGeom, rcConfig config, int *const pEdges, const int edgeSize) {
  // load in actual svg file
  const float *min = pGeom.getMeshBoundsMin();
  const float inverseSellSize{1.0f / config.cs};
  std::set<Edge> referenceEdgesSet;
  std::ifstream csfFileRef{input};

  std::string line;
  // Read each line from the file
  while (std::getline(csfFileRef, line)) {
    std::stringstream ss(line);
    std::string cell;
    std::vector<int> row{};
    // Split the line into cells using a comma as a delimiter and convert to integers
    while (std::getline(ss, cell, ',')) {
      float value = std::stof(cell);
      if ((row.size() & 3u) == 0u || (row.size() & 3u) == 2u) {
        value -= min[0];
      } else {
        value -= min[2];
      }
      row.push_back(static_cast<int>(value * inverseSellSize));
    }
    if (row[0] > row[2] || (row[0] == row[2] && row[1] > row[3])) {
      std::swap(row[0], row[2]);
      std::swap(row[1], row[3]);
    }
    int y1 = config.height - row[1];
    int y2 = config.height - row[3];
    if(y1 > config.height || y2 > config.height)
    {
      y1 = y1 - config.height;
      y2 = y2 - config.height;
    }
    if(y1 < 0 || y2 < 0)
    {
      y1 = config.height - abs(y1);
      y2 = config.height - abs(y2);
    }
    referenceEdgesSet.emplace(Edge{Vertex{row[0], y1}, Vertex{row[2], y2}});
  }
  csfFileRef.close();
  std::set<Edge> resultEdgesSet{};
  for (int i = 0; i < edgeSize / 2 - 1; i += 2) {
    const int ii = i + 1;
    if (pEdges[i * 2 + 0] > pEdges[ii * 2 + 0] || (pEdges[i * 2 + 0] == pEdges[ii * 2 + 0] && pEdges[i * 2 + 1] > pEdges[ii * 2 + 1])) {
      resultEdgesSet.emplace(Edge{{pEdges[ii * 2 + 0], pEdges[ii * 2 + 1]}, {pEdges[i * 2 + 0], pEdges[i * 2 + 1]}});
    } else {
      resultEdgesSet.emplace(Edge{{pEdges[i * 2 + 0], pEdges[i * 2 + 1]}, {pEdges[ii * 2 + 0], pEdges[ii * 2 + 1]}});
    }
  }
  rcFree(pEdges);

  std::vector<Edge> referenceEdges{};
  std::vector<Edge> resultEdges{};
  std::copy(referenceEdgesSet.cbegin(), referenceEdgesSet.cend(), std::back_inserter(referenceEdges));
  std::copy(resultEdgesSet.cbegin(), resultEdgesSet.cend(), std::back_inserter(resultEdges));

  std::ofstream svg;
  startSvg(svg, output + "/edges_" + name + "_result.svg", config.width, config.height);
  writeSvgLine(svg, resultEdges, "black");

  endSvg(svg);
  startSvg(svg, output + "/edges_" + name + "_reference.svg", config.width, config.height);
  writeSvgLine(svg, referenceEdges, "black");
  endSvg(svg);

  const uint16_t epsilon{static_cast<uint16_t>(std::ceil(inverseSellSize))};

  const auto moveMatch{
      [epsilon](const Edge &e1, const Edge &e2) -> bool {
        if (e1.v1.x == e2.v1.x && e1.v1.y == e2.v1.y &&
            e1.v2.x == e2.v2.x && e1.v2.y == e2.v2.y)
          return true;

        const int diffX1 = e1.v1.x - e2.v1.x;
        const int diffY1 = e1.v1.y - e2.v1.y;
        const int diffX2 = e1.v2.x - e2.v2.x;
        const int diffY2 = e1.v2.y - e2.v2.y;
        const int diffX3 = e1.v1.x - e2.v2.x;
        const int diffY3 = e1.v1.y - e2.v2.y;
        const int diffX4 = e1.v2.x - e2.v1.x;
        const int diffY4 = e1.v2.y - e2.v1.y;
        const int smallestDiffX1 = std::abs(diffX1) < std::abs(diffX3) ? diffX1 : diffX3;
        const int smallestDiffX2 = std::abs(diffX2) < std::abs(diffX4) ? diffX2 : diffX4;
        const int smallestDiffY1 = std::abs(diffY1) < std::abs(diffY3) ? diffY1 : diffY3;
        const int smallestDiffY2 = std::abs(diffY2) < std::abs(diffY4) ? diffY2 : diffY4;
        // Compare the squared length of the difference with the squared epsilon
        if (smallestDiffX1 * smallestDiffX1 + smallestDiffY1 * smallestDiffY1 <= epsilon * epsilon && smallestDiffX2 * smallestDiffX2 + smallestDiffY2 * smallestDiffY2 <= epsilon * epsilon)
          return true;
        return false;
      }};
  const std::size_t referenceEdgesSize = referenceEdges.size();
  uint32_t tp{};
  uint32_t fp{};
  std::vector<Edge> falsePositive{};
  std::vector<Edge> truePositive{};
  for (const Edge &edge1 : resultEdges) {
    bool found = false;
    std::sort(referenceEdges.begin(), referenceEdges.end(), [edge1](const Edge &edgeA, const Edge &edgeB) -> bool {
      const auto distance{
          [](const Edge &e1, const Edge &e2) -> int32_t {
            const int diffX1 = e1.v1.x - e2.v1.x;
            const int diffY1 = e1.v1.y - e2.v1.y;
            const int diffX2 = e1.v2.x - e2.v2.x;
            const int diffY2 = e1.v2.y - e2.v2.y;
            const int diffX3 = e1.v1.x - e2.v2.x;
            const int diffY3 = e1.v1.y - e2.v2.y;
            const int diffX4 = e1.v2.x - e2.v1.x;
            const int diffY4 = e1.v2.y - e2.v1.y;
            const int smallestDiffX1 = std::abs(diffX1) < std::abs(diffX3) ? diffX1 : diffX3;
            const int smallestDiffX2 = std::abs(diffX2) < std::abs(diffX4) ? diffX2 : diffX4;
            const int smallestDiffY1 = std::abs(diffY1) < std::abs(diffY3) ? diffY1 : diffY3;
            const int smallestDiffY2 = std::abs(diffY2) < std::abs(diffY4) ? diffY2 : diffY4;

            const int halfDiffX = (smallestDiffX1 + smallestDiffX2) / 2;
            const int halfDiffY = (smallestDiffY1 + smallestDiffY2) / 2;
            return halfDiffX * halfDiffX + halfDiffY * halfDiffY;
          }};

      return distance(edge1, edgeA) < distance(edge1, edgeB);
    });
    for (const Edge &edge2 : referenceEdges) {
      if (moveMatch(edge1, edge2)) {
        found = true;
        auto it = std::find_if(referenceEdges.begin(), referenceEdges.end(), [edge2](const Edge &edge) {
          return edge.v1.x == edge2.v1.x && edge.v1.y == edge2.v1.y && edge.v2.x == edge2.v2.x && edge.v2.y == edge2.v2.y;
        });
        referenceEdges.erase(it);
        break;
      }
    }
    if (found) {
      truePositive.push_back(edge1);
      ++tp;
    } else {
      ++fp;
      falsePositive.push_back(edge1);
    }
  }

  float const precision = static_cast<float>(tp) / static_cast<float>(tp + fp);
  float const recall = static_cast<float>(tp) / static_cast<float>(referenceEdgesSize);

  startSvg(svg, output + "/edges_" + name + "_leftover.svg", config.width, config.height);
  writeSvgLine(svg, referenceEdges, "black");
  writeSvgLine(svg, falsePositive, "red");
  writeSvgLine(svg, truePositive, "green");
  svg << "<text x=\"5\" y=\"15\" fill=\"black\"> True Positives: " << tp << "    False Positives: " << fp << "    Precision: " << precision << "    Recall: " << recall << "</text>\n";
  endSvg(svg);
}
