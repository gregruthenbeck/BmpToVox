///  @file	VertPool.h
///  @brief	Implements class: VertPool
///
///		TODO: File description (delete me?)
///
///		Copyright 2011 Greg Ruthenbeck. Flinders University. Australia
///
///  @author	Greg Ruthenbeck
///  @version	0.1
///  Originally created:   April 2011

#pragma once

//#include <DXUT.h>
//#include <nvMath.h>
#include <vector>
#include <set>
#include <map>
#include <utility> // std::pair
#include <iostream>

//struct Vec3;
//class TriDX; // defined in Utils.h

//class IMT_VERTEX
//{
//public:
//	IMT_VERTEX()  {}
//	IMT_VERTEX(const nv::vec3f& p, const nv::vec3f& n) 
//		: pos(p.x, p.y, p.z), norm(n.x, n.y, n.z) {}
//	IMT_VERTEX(const nv::vec3f& p, const Vec3& n) 
//		: pos(p.x, p.y, p.z), norm(n) {}
//	IMT_VERTEX(const Vec3& p, const Vec3& n) 
//		: pos(p), norm(n) {}
//	IMT_VERTEX(const nv::vec3f& p) 
//		: pos(p.x, p.y, p.z) {}
//	Vec3 pos;
//	Vec3 norm;
//};


//#define LOG_ERROR(x) { std::cout << "ERR " << x << std::endl; }
//#define LOG(x)		 { std::cout << "INF " << x << std::endl; }
//#define LOG_WARN(x)  { std::cout << "WAR " << x << std::endl; }

typedef std::set<unsigned int> SetUInt;
typedef std::map<unsigned int, SetUInt > MapUIntToSetUInt;
typedef std::set<unsigned int> SetUShort;
typedef unsigned int KeyType; // mPoolDim up to 1024 use KeyType uint. 2048 and above use KeyType ulong
typedef unsigned char RefCountType; 
typedef unsigned int VertIdType; 

class Vec3
{
public:
	Vec3(){}
	Vec3(const float _x, const float _y, const float _z)
		: x(_x), y(_y), z(_z) {}

	float x,y,z;

	inline Vec3 operator + ( const Vec3& rkVector ) const
	{
		return Vec3(x + rkVector.x,
					y + rkVector.y,
					z + rkVector.z);
	}
	inline Vec3 operator - ( const Vec3& rkVector ) const
	{
		return Vec3(x - rkVector.x,
					y - rkVector.y,
					z - rkVector.z);
	}
	inline Vec3 operator * ( const float f ) const
	{
		return Vec3(x * f,
					y * f,
					z * f);
	}
};

struct SIMPLE_VERTEX
{
	SIMPLE_VERTEX() {}
	SIMPLE_VERTEX(const Vec3& p)
		: pos(p) {}
	Vec3 pos, norm;
};

template<class VertT>
class VertPool
{
public:
	VertPool(const KeyType dim, const Vec3& minVert, const Vec3& span);
	const VertIdType& AddVertRef(const VertT& v);
	const std::vector<VertT>& GetPooledVerts() const { return mVerts; }
	void RemoveVertRef(const VertIdType& vId);
	void RemoveVertRef(const Vec3& pos);

protected:
	unsigned int GetKey(const Vec3& p) const;
	unsigned int GetKey(const float x, const float y, const float z) const;

	std::vector<VertT> mVerts;

	std::map<KeyType, std::pair<VertIdType, RefCountType> > mRefs;
	std::vector<VertIdType> mAvailable;

	const KeyType   mDim;
	const float     mDimFlt;
	const Vec3 mSpan;
	const Vec3 mMin;
};


template<class VertT>
VertPool<VertT>::VertPool(const KeyType dim, const Vec3& minVert, const Vec3& span)
: mDim(dim), 
  mDimFlt(static_cast<float>(dim)), 
  mMin(minVert),
  mSpan(span)
{
}

template<class VertT>
const VertIdType& VertPool<VertT>::AddVertRef(const VertT& v)
{
	const KeyType key = GetKey(v.pos);
	map<KeyType, std::pair<VertIdType, RefCountType> >::iterator iPRef = mRefs.find(key);

	// If we don't have a nearby vert already in the pool
	if (iPRef == mRefs.end())
	{
		// If we don't have any locations in the vert-pool available
		if (mAvailable.empty())
		{
			mRefs[key] = make_pair(mVerts.size(), 1); // insert an entry into the vert-pool
			mVerts.push_back(v);
		}
		else
		{
			// Re-use an existing location in the vert-pool
			const VertIdType newVertId = mAvailable.back();
			mAvailable.pop_back();

			mRefs[key] = make_pair(newVertId, 1);
			mVerts[newVertId] = v;
		}
	}
	else
		++(iPRef->second.second); // increment the reference count for the vert

	const VertIdType& vertIndex = mRefs[key].first;

	// Average the existing normal (and position) with the newly referenced normal (and position)
	std::vector<VertT>::iterator vIter = mVerts.begin() + vertIndex;
	vIter->pos  = (vIter->pos + v.pos) * 0.5f;
	//vIter->norm = (vIter->norm + v.norm) * 0.5f; // Could weight this average on TriDX area	

	return vertIndex;
}


template<class VertT>
void VertPool<VertT>::RemoveVertRef(const VertIdType& vId)
{
	RemoveVertRef(mVerts[vId].pos);
}

template<class VertT>
void VertPool<VertT>::RemoveVertRef(const Vec3& pos)
{
	const KeyType key = GetKey(pos);
	map<KeyType, std::pair<VertIdType, RefCountType> >::iterator iPRef = mRefs.find(key);

	if (iPRef != mRefs.end())
	{
		RefCountType& refCount = iPRef->second.second;

		if (refCount != 0) // don't wrap past zero
			--refCount;
		else
			LOG_WARN("Ignoring attempt to decrement ref count below zero");

		if (refCount == 0)
		{
			// *** Delete the vert from the pool
			// Find the ID of the vert in the map
			map<KeyType, std::pair<VertIdType, RefCountType> >::iterator i = mRefs.find(key);
			// Store the ID of the vert for re-use
			mAvailable.push_back(i->second.first);
			// Delete the vert from the ref map
			mRefs.erase(i);
		}
	}
	else
		LOG_WARN("Key not found!");
}

template<class VertT>
unsigned int VertPool<VertT>::GetKey(const float x, const float y, const float z) const
{
	const KeyType xi = static_cast<KeyType>(((x + mMin.x) / mSpan.x/* + 0.5f*/) * mDimFlt) % mDim;
	const KeyType yi = static_cast<KeyType>(((y + mMin.y) / mSpan.y/* + 0.5f*/) * mDimFlt) % mDim;
	const KeyType zi = static_cast<KeyType>(((z + mMin.z) / mSpan.z/* + 0.5f*/) * mDimFlt) % mDim;

	return zi * (mDim * mDim) + yi * mDim + xi;
}

//template<class VertT>
//unsigned int VertPool<VertT>::GetKey(const Vec3* const p) const
//{
//	return GetKey(p->x, p->y, p->z);
//}
//
template<class VertT>
unsigned int VertPool<VertT>::GetKey(const Vec3& p) const
{
	return GetKey(p.x, p.y, p.z);
}


