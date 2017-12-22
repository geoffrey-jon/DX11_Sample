/*  ======================

	======================  */

#ifndef RENDERPASS_H
#define RENDERPASS_H

class RenderPass
{
public:
	RenderPass() {}
	~RenderPass() {}

	virtual void Init() = 0;
	virtual void Update(float dt) = 0;
	virtual void Draw() = 0;

private:

};

#endif // RENDERPASS_H