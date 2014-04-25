// ======================================================================== //
// Copyright 2009-2013 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "bvh4hair.h"
#include "bvh4hair_builder2.h"
#include "bvh4hair_statistics.h"
#include "common/scene_bezier_curves.h"

namespace embree
{
  extern double g_hair_builder_replication_factor;
 
  BVH4HairBuilder2::BVH4HairBuilder2 (BVH4Hair* bvh, Scene* scene)
    : scene(scene), minLeafSize(1), maxLeafSize(inf), bvh(bvh), remainingReplications(0)
  {
    if (BVH4Hair::maxLeafBlocks < this->maxLeafSize) 
      this->maxLeafSize = BVH4Hair::maxLeafBlocks;

    enableAlignedObjectSplits = false;
    enableAlignedSpatialSplits = false;
    enableUnalignedObjectSplits = false;
    enableStrandSplits = false;
    enablePreSubdivision = 0;
    
    for (size_t i=0; i<g_hair_accel_mode.size();)
    {
      if      (g_hair_accel_mode.substr(i,2) == "P0" ) { enablePreSubdivision = 0; i+=2; } 
      else if (g_hair_accel_mode.substr(i,2) == "P1" ) { enablePreSubdivision = 1; i+=2; } 
      else if (g_hair_accel_mode.substr(i,2) == "P2" ) { enablePreSubdivision = 2; i+=2; } 
      else if (g_hair_accel_mode.substr(i,2) == "P3" ) { enablePreSubdivision = 3; i+=2; } 
      else if (g_hair_accel_mode.substr(i,2) == "P4" ) { enablePreSubdivision = 4; i+=2; } 
      else if (g_hair_accel_mode.substr(i,2) == "aO" ) { enableAlignedObjectSplits = true; i+=2; } 
      else if (g_hair_accel_mode.substr(i,2) == "uO" ) { enableUnalignedObjectSplits = true; i+=2; } 
      else if (g_hair_accel_mode.substr(i,3) == "auO" ) { enableAlignedObjectSplits = enableUnalignedObjectSplits = true; i+=3; } 
      else if (g_hair_accel_mode.substr(i,3) == "uST") { enableStrandSplits = true; i+=3; } 
      else if (g_hair_accel_mode.substr(i,3) == "aSP") { enableAlignedSpatialSplits = true; i+=3; } 
      else throw std::runtime_error("invalid hair accel mode");
    }
  }

  void BVH4HairBuilder2::build(size_t threadIndex, size_t threadCount) 
  {
    /* fast path for empty BVH */
    size_t numPrimitives = scene->numCurves << enablePreSubdivision;
    bvh->init(numPrimitives,numPrimitives+(size_t)(g_hair_builder_replication_factor*numPrimitives));
    if (numPrimitives == 0) return;
    numGeneratedPrims = 0;

    double t0 = 0.0;
    if (g_verbose >= 2) 
    {
      PRINT(enableAlignedObjectSplits);
      PRINT(enableAlignedSpatialSplits);
      PRINT(enableUnalignedObjectSplits);
      PRINT(enableStrandSplits);
      PRINT(enablePreSubdivision);

      std::cout << "building BVH4Hair<" + bvh->primTy.name + "> using BVH4HairBuilder2 ..." << std::flush;
      t0 = getSeconds();
    }

    size_t N = 0;
    float r = 0;

    /* create initial curve list */
    BBox3fa bounds = empty;
    size_t numVertices = 0;
    BezierRefList prims;
    for (size_t i=0; i<scene->size(); i++) 
    {
      Geometry* geom = scene->get(i);
      if (geom->type != BEZIER_CURVES) continue;
      if (!geom->isEnabled()) continue;
      BezierCurves* set = (BezierCurves*) geom;
      numVertices += set->numVertices;
      for (size_t j=0; j<set->numCurves; j++) {
        const int ofs = set->curve(j);
        const Vec3fa& p0 = set->vertex(ofs+0);
        const Vec3fa& p1 = set->vertex(ofs+1);
        const Vec3fa& p2 = set->vertex(ofs+2);
        const Vec3fa& p3 = set->vertex(ofs+3);
        const Bezier1 bezier(p0,p1,p2,p3,0,1,i,j);
        bounds.extend(subdivideAndAdd(threadIndex,prims,bezier,enablePreSubdivision));
      }
    }

    /* compute primitive info */
    PrimInfo pinfo;
    for (BezierRefList::block_iterator_unsafe i = prims; i; i++) 
      pinfo.add((*i).bounds(),(*i).center());

    bvh->numPrimitives = scene->numCurves;
    bvh->numVertices = 0;
    if (&bvh->primTy == &SceneBezier1i::type) bvh->numVertices = numVertices;

    /* start recursive build */
    remainingReplications = g_hair_builder_replication_factor*numPrimitives;
    BuildTask task(&bvh->root,0,pinfo,prims,pinfo.geomBounds);
    bvh->bounds = bounds;

#if 0
    recurseTask(threadIndex,task);
#else
    numActiveTasks = 1;
    tasks.push_back(task);
    push_heap(tasks.begin(),tasks.end());
    TaskScheduler::executeTask(threadIndex,threadCount,_task_build_parallel,this,threadCount,"BVH4Builder::build_parallel");
#endif
    
    if (g_verbose >= 2) {
      double t1 = getSeconds();
      std::cout << " [DONE]" << std::endl;
      std::cout << "  dt = " << 1000.0f*(t1-t0) << "ms, perf = " << 1E-6*double(numPrimitives)/(t1-t0) << " Mprim/s" << std::endl;
      std::cout << BVH4HairStatistics(bvh).str();
    }
  }

  const BBox3fa BVH4HairBuilder2::subdivideAndAdd(size_t threadIndex, BezierRefList& prims, const Bezier1& bezier, size_t depth)
  {
    if (depth == 0) {
      BezierRefList::item* block = prims.head();
      if (block == NULL || !block->insert(bezier)) {
        block = prims.insert(alloc.malloc(threadIndex));
        block->insert(bezier);
      }
      return bezier.bounds();
    }

    Bezier1 bezier0,bezier1;
    bezier.subdivide(bezier0,bezier1);
    const BBox3fa bounds0 = subdivideAndAdd(threadIndex,prims,bezier0,depth-1);
    const BBox3fa bounds1 = subdivideAndAdd(threadIndex,prims,bezier1,depth-1);
    return merge(bounds0,bounds1);
  }

  void BVH4HairBuilder2::task_build_parallel(size_t threadIndex, size_t threadCount, size_t taskIndex, size_t taskCount, TaskScheduler::Event* event) 
  {
    while (numActiveTasks) 
    {
      taskMutex.lock();
      if (tasks.size() == 0) {
        taskMutex.unlock();
        continue;
      }

      /* take next task from heap */
      BuildTask task = tasks.front();
      pop_heap(tasks.begin(),tasks.end());
      tasks.pop_back();
      taskMutex.unlock();

      /* recursively finish task */
      if (task.pinfo.size() < 512) {
        atomic_add(&numActiveTasks,-1);
        recurseTask(threadIndex,task);
      }
      
      /* execute task and add child tasks */
      else 
      {
        size_t numChildren;
        BuildTask ctasks[BVH4Hair::N];
        processTask(threadIndex,task,ctasks,numChildren);
        taskMutex.lock();
        for (size_t i=0; i<numChildren; i++) {
          atomic_add(&numActiveTasks,+1);
          tasks.push_back(ctasks[i]);
          push_heap(tasks.begin(),tasks.end());
        }
        atomic_add(&numActiveTasks,-1);
        taskMutex.unlock();
      }
    }
  }

  const NAABBox3fa BVH4HairBuilder2::computeHairSpaceBounds(BezierRefList& prims)
  {
    size_t N = BezierRefList::block_iterator_unsafe(prims).size();
    if (N == 0)
      return NAABBox3fa(empty); // FIXME: can cause problems with compression

    float bestArea = inf;
    LinearSpace3fa bestSpace = one;
    BBox3fa bestBounds = empty;

    size_t k=0;
    for (BezierRefList::block_iterator_unsafe i = prims; i; i++)
    {
      //if ((k++) % ((N+1)/2)) continue;
      if ((k++) % ((N+3)/4)) continue;
      //if ((k++) % ((N+15)/16)) continue;
      const Vec3fa axis = normalize(i->p3 - i->p0);
      if (length(i->p3 - i->p0) < 1E-9) continue;
      const LinearSpace3fa space = clamp(frame(axis).transposed());
      BBox3fa bounds = empty;
      float area = 0.0f;
      for (BezierRefList::block_iterator_unsafe j = prims; j; j++) {
        const BBox3fa cbounds = j->bounds(space);
        area += halfArea(cbounds);
        bounds.extend(cbounds);
      }

      if (area <= bestArea) {
        bestBounds = bounds;
        bestSpace = space;
        bestArea = area;
      }
    }

    /* select world space for some corner cases */
    if (bestArea == float(inf)) 
    {
      bestSpace = one;
      bestBounds = empty;
      for (BezierRefList::block_iterator_unsafe j = prims; j; j++)
        bestBounds.extend(j->bounds());
    }

    return NAABBox3fa(bestSpace,bestBounds);
  }

  BVH4Hair::NodeRef BVH4HairBuilder2::leaf(size_t threadIndex, size_t depth, BezierRefList& prims, const NAABBox3fa& bounds)
  {
    //size_t N = end-begin;
    size_t N = BezierRefList::block_iterator_unsafe(prims).size();

    if (N > (size_t)BVH4Hair::maxLeafBlocks) {
      //std::cout << "WARNING: Loosing " << N-BVH4Hair::maxLeafBlocks << " primitives during build!" << std::endl;
      std::cout << "!" << std::flush;
      N = (size_t)BVH4Hair::maxLeafBlocks;
    }
    size_t numGeneratedPrimsOld = atomic_add(&numGeneratedPrims,N); 
    if (numGeneratedPrimsOld%10000 > (numGeneratedPrimsOld+N)%10000) std::cout << "." << std::flush; 
    //assert(N <= (size_t)BVH4Hair::maxLeafBlocks);
    if (&bvh->primTy == &Bezier1Type::type) {
      Bezier1* leaf = (Bezier1*) bvh->allocPrimitiveBlocks(threadIndex,N);
      BezierRefList::block_iterator_unsafe iter(prims);
      for (size_t i=0; i<N; i++) { leaf[i] = *iter; iter++; }
      assert(!iter);

      /* free all primitive blocks */
      while (BezierRefList::item* block = prims.take())
        alloc.free(threadIndex,block);

      return bvh->encodeLeaf((char*)leaf,N);
    } 
    else if (&bvh->primTy == &SceneBezier1i::type) {
      Bezier1i* leaf = (Bezier1i*) bvh->allocPrimitiveBlocks(threadIndex,N);
      BezierRefList::block_iterator_unsafe iter(prims);
      for (size_t i=0; i<N; i++) {
        const Bezier1& curve = *iter; iter++;
        const BezierCurves* in = (BezierCurves*) scene->get(curve.geomID);
        const Vec3fa& p0 = in->vertex(in->curve(curve.primID));
        leaf[i] = Bezier1i(&p0,curve.geomID,curve.primID,-1); // FIXME: support mask
      }

      /* free all primitive blocks */
      while (BezierRefList::item* block = prims.take())
        alloc.free(threadIndex,block);

      return bvh->encodeLeaf((char*)leaf,N);
    }
    else 
      throw std::runtime_error("unknown primitive type");
  }

  void BVH4HairBuilder2::split(size_t threadIndex, 
                               BezierRefList& prims, const NAABBox3fa& bounds, const PrimInfo& pinfo,
                               BezierRefList& lprims_o, PrimInfo& linfo_o, 
                               BezierRefList& rprims_o, PrimInfo& rinfo_o, 
                               bool& isAligned)
  {
    /* variable to track the SAH of the best splitting approach */
    float bestSAH = inf;
    const float leafSAH = BVH4Hair::intCost*float(pinfo.size())*halfArea(bounds.bounds);
    
    /* perform standard binning in aligned space */
    ObjectPartition::Split alignedObjectSplit;
    float alignedObjectSAH = inf;
    if (enableAlignedObjectSplits) {
      alignedObjectSplit = ObjectPartition::find(threadIndex,prims,one);
      alignedObjectSAH = BVH4Hair::travCostAligned*halfArea(bounds.bounds) + alignedObjectSplit.splitSAH(BVH4Hair::intCost);
      bestSAH = min(bestSAH,alignedObjectSAH);
    }

    /* perform spatial split in aligned space */
    SpatialSplit::Split alignedSpatialSplit;
    float alignedSpatialSAH = inf;
    bool enableSpatialSplits = remainingReplications > 0;
    if (enableSpatialSplits && enableAlignedSpatialSplits) {
      alignedSpatialSplit = SpatialSplit::find(threadIndex,prims,pinfo);
      alignedSpatialSAH = BVH4Hair::travCostAligned*halfArea(bounds.bounds) + BVH4Hair::intCost*alignedSpatialSplit.splitSAH();
      bestSAH = min(bestSAH,alignedSpatialSAH);
    }

    /* perform standard binning in unaligned space */
    ObjectPartition::Split unalignedObjectSplit;
    float unalignedObjectSAH = inf;
    if (enableUnalignedObjectSplits) {
      unalignedObjectSplit = ObjectPartition::find(threadIndex,prims,bounds.space);
      unalignedObjectSAH = BVH4Hair::travCostUnaligned*halfArea(bounds.bounds) + unalignedObjectSplit.splitSAH(BVH4Hair::intCost);
      bestSAH = min(bestSAH,unalignedObjectSAH);
    }

    /* perform splitting into two strands */
    StrandSplit strandSplit;
    float strandSAH = inf;
    if (enableStrandSplits) {
      strandSplit = StrandSplit::find(threadIndex,prims);
      strandSAH = BVH4Hair::travCostUnaligned*halfArea(bounds.bounds) + strandSplit.splitSAH(BVH4Hair::intCost);
      bestSAH = min(bestSAH,strandSAH);
    }

    /* perform fallback split */
    if (bestSAH == float(inf)) {
      FallBackSplit::find(threadIndex,alloc,prims,lprims_o,linfo_o,rprims_o,rinfo_o);
    }

    /* perform aligned object split */
    else if (bestSAH == alignedObjectSAH) {
      alignedObjectSplit.split(threadIndex,alloc,prims,lprims_o,linfo_o,rprims_o,rinfo_o);
    }

    /* perform aligned spatial split */
    else if (bestSAH == alignedSpatialSAH) {
      alignedSpatialSplit.split(threadIndex,alloc,prims,lprims_o,linfo_o,rprims_o,rinfo_o);
      atomic_add(&remainingReplications,pinfo.size()-linfo_o.size()-rinfo_o.size());
    }

    /* perform unaligned object split */
    else if (bestSAH == unalignedObjectSAH) {
      unalignedObjectSplit.split(threadIndex,alloc,prims,lprims_o,linfo_o,rprims_o,rinfo_o);
      isAligned = false;
    }

    /* perform strand split */
    else if (bestSAH == strandSAH) {
      strandSplit.split(threadIndex,alloc,prims,lprims_o,linfo_o,rprims_o,rinfo_o);
      isAligned = false;
    }
 
    else {
      throw std::runtime_error("bvh4hair_builder: internal error");
    }
  }

  void BVH4HairBuilder2::processTask(size_t threadIndex, BuildTask& task, BuildTask task_o[BVH4Hair::N], size_t& numTasks_o)
  {
    /* create enforced leaf */
    if (task.pinfo.size() <= minLeafSize || task.depth >= BVH4Hair::maxBuildDepth) {
      *task.dst = leaf(threadIndex,task.depth,task.prims,task.bounds);
      numTasks_o = 0;
      return;
    }

    /*! initialize child list */
    bool isAligned = true;
    PrimInfo cpinfo[BVH4Hair::N];
    NAABBox3fa cbounds[BVH4Hair::N];
    BezierRefList cprims[BVH4Hair::N];
    cprims[0] = task.prims;
    cbounds[0] = task.bounds;
    cpinfo[0] = task.pinfo;
    size_t numChildren = 1;
    
    /*! split until node is full or SAH tells us to stop */
    do {
      
      /*! find best child to split */
      float bestArea = neg_inf; 
      ssize_t bestChild = -1;
      for (size_t i=0; i<numChildren; i++) 
      {
        float A = halfArea(cbounds[i].bounds);
        if (cpinfo[i].size() <= minLeafSize) continue;  
        if (A > bestArea) { bestChild = i; bestArea = A; }
      }
      if (bestChild == -1) break;

      /*! split selected child */
      PrimInfo linfo, rinfo;
      BezierRefList lprims, rprims;
      split(threadIndex,cprims[bestChild],cbounds[bestChild],cpinfo[bestChild],lprims,linfo,rprims,rinfo,isAligned);
      cprims[numChildren] = rprims; cpinfo[numChildren] = rinfo;
      cprims[bestChild  ] = lprims; cpinfo[bestChild  ] = linfo;
      cbounds[numChildren] = computeHairSpaceBounds(cprims[numChildren]);
      cbounds[bestChild  ] = computeHairSpaceBounds(cprims[bestChild  ]);
      numChildren++;
      
    } while (numChildren < BVH4Hair::N);

    /* create aligned node */
    if (isAligned) 
    {
      BVH4Hair::AlignedNode* node = bvh->allocAlignedNode(threadIndex);
      for (ssize_t i=0; i<numChildren; i++) {
        node->set(i,cpinfo[i].geomBounds);
	const NAABBox3fa ubounds = computeHairSpaceBounds(cprims[i]);
        new (&task_o[i]) BuildTask(&node->child(i),task.depth+1,cpinfo[i],cprims[i],ubounds);
      }
      numTasks_o = numChildren;
      *task.dst = bvh->encodeNode(node);
    }
    
    /* create unaligned node */
    else {
      BVH4Hair::UnalignedNode* node = bvh->allocUnalignedNode(threadIndex);
      for (ssize_t i=numChildren-1; i>=0; i--) {
        node->set(i,cbounds[i]);
        new (&task_o[i]) BuildTask(&node->child(i),task.depth+1,cpinfo[i],cprims[i],cbounds[i]);
      }
      numTasks_o = numChildren;
      *task.dst = bvh->encodeNode(node);
    }
  }

  void BVH4HairBuilder2::recurseTask(size_t threadIndex, BuildTask& task)
  {
    size_t numChildren;
    BuildTask tasks[BVH4Hair::N];
    processTask(threadIndex,task,tasks,numChildren);
    for (size_t i=0; i<numChildren; i++) 
      recurseTask(threadIndex,tasks[i]);
  }

  Builder* BVH4HairBuilder2_ (BVH4Hair* accel, Scene* scene) {
    return new BVH4HairBuilder2(accel,scene);
  }
}
