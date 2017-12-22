#include "GObjectStore.h"

GObjectStore::GObjectStore()
{
}

GObjectStore::~GObjectStore()
{

}

void GObjectStore::AddObject(GObject* obj)
{
	mObjects.push_back(obj);
}

std::vector<GObject*> GObjectStore::GetObjects()
{
	return mObjects;
}