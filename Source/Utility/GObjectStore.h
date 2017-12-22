/*  ======================

	======================  */

#ifndef G_OBJECTSTORE_H
#define G_OBJECTSTORE_H

#include <vector>
#include "GObject.h"

class GObjectStore
{
public:
	GObjectStore();
	~GObjectStore();

	void AddObject(GObject* obj);
	std::vector<GObject*> GetObjects();

private:
	std::vector<GObject*> mObjects;
};

#endif // G_OBJECTSTORE_H