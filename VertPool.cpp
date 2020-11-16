///  @file	VertPool.cpp
///  @brief	Implements class: VertPool
///
///		Pools verts for re-use using a std::map. If a new point (Vec3) is near
///     an old one, return the old index. Else, return a new index and insert
///     the vert into the pool.
/// 
///		Uses min, span, and dim to set the behaviour of "GetKey(pos)" for deciding
///     what is "near".
///
///		Copyright 2011 Greg Ruthenbeck
///
///  @author	Greg Ruthenbeck
///  @version	0.1
///  Originally created:   April 2011

#include "VertPool.h"

// EOF