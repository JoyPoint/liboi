/*
 * CRoutine_ImageToBuffer.cpp
 *
 *  Created on: Nov 18, 2011
 *      Author: bkloppenborg
 */

#include "CRoutine_ImageToBuffer.h"

CRoutine_ImageToBuffer::CRoutine_ImageToBuffer(cl_device_id device, cl_context context, cl_command_queue queue)
	:COpenCLRoutine(device, context, queue)
{
	mSource.push_back("image2buf_GL_R.cl");

}

CRoutine_ImageToBuffer::~CRoutine_ImageToBuffer()
{
	// TODO Auto-generated destructor stub
}

// Read in the kernel source and build program object.
void CRoutine_ImageToBuffer::Init()
{
	string source = ReadSource(mSource[0]);
	BuildKernel(source);
}

void CRoutine_ImageToBuffer::CopyImage(cl_mem gl_image, cl_mem cl_buffer, int width, int height, int depth)
{
	// Create the image size memory object
	cl_int2 tmp;
	tmp.x = width;
	tmp.y = height;

	// TODO: We need to redo this for 3D data sets and for non-square images.
	// TODO: Figure out how to determine these sizes dynamically.
	size_t * global = new size_t[2];
	global[0] = global[1] = width;
	size_t * local = new size_t[2];
	local[0] = local[1] = 16;

	// Enqueue the kernel.
	int err = CL_SUCCESS;
    err |= clSetKernelArg(mKernels[0],  0, sizeof(cl_mem), &gl_image);
    err |= clSetKernelArg(mKernels[0],  1, sizeof(cl_mem), &cl_buffer);
    err |= clSetKernelArg(mKernels[0],  2, sizeof(cl_int2), &tmp);
	COpenCL::CheckOCLError("Failed to set gl_image to cl_buffer kernel arguments.", err);


    err = CL_SUCCESS;
    err |= clEnqueueNDRangeKernel(mQueue, mKernels[0], 2, NULL, global, local, 0, NULL, NULL);
    COpenCL::CheckOCLError("Failed to enqueue image copying kernel.", err);

    // Free memory
    delete global;
    delete local;
}
