/*
 * CRoutine_FTtoT3.cpp
 *
 *  Created on: Dec 5, 2011
 *      Author: bkloppenborg
 *
 *  Description:
 *      Routine to convert a Fourier transformed data into bispectra.
 */

 /* 
 * Copyright (c) 2012 Brian Kloppenborg
 *
 * If you use this software as part of a scientific publication, please cite as:
 *
 * Kloppenborg, B.; Baron, F. (2012), "LibOI: The OpenCL Interferometry Library"
 * (Version X). Available from  <https://github.com/bkloppenborg/liboi>.
 *
 * This file is part of the OpenCL Interferometry Library (LIBOI).
 * 
 * LIBOI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License 
 * as published by the Free Software Foundation, either version 3 
 * of the License, or (at your option) any later version.
 * 
 * LIBOI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public 
 * License along with LIBOI.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CRoutine_FTtoT3.h"
#include "COILibData.h"
#include <cstdio>
#include <complex>

namespace liboi
{

CRoutine_FTtoT3::CRoutine_FTtoT3(cl_device_id device, cl_context context, cl_command_queue queue)
	:CRoutine(device, context, queue)
{
	// Specify the source location for the kernel.
	mSource.push_back("ft_to_t3.cl");
}

CRoutine_FTtoT3::~CRoutine_FTtoT3()
{
	// TODO Auto-generated destructor stub
}

void CRoutine_FTtoT3::Init(void)
{
	// Read the kernel, compile it
	string source = ReadSource(mSource[0]);
    BuildKernel(source, "ft_to_t3", mSource[0]);
}

void CRoutine_FTtoT3::FTtoT3(cl_mem ft_input, cl_mem t3_uv_ref, cl_mem t3_uv_sign, cl_mem output, int n_vis, int n_v2, int n_t3)
{
	if(n_t3 == 0)
		return;

	int status = 0;
	size_t global = (size_t) n_t3;
	size_t local = 0;

	// Get the maximum work-group size for executing the kernel on the device
	status = clGetKernelWorkGroupInfo(mKernels[0], mDeviceID, CL_KERNEL_WORK_GROUP_SIZE , sizeof(size_t), &local, NULL);
	CHECK_OPENCL_ERROR(status, "clGetKernelWorkGroupInfo failed.");

	unsigned int offset = COILibData::CalculateOffset_T3(n_vis, n_v2);

	// Set kernel arguments:
	status  = clSetKernelArg(mKernels[0], 0, sizeof(cl_mem), &ft_input);
	status |= clSetKernelArg(mKernels[0], 1, sizeof(cl_mem), &t3_uv_ref);
	status |= clSetKernelArg(mKernels[0], 2, sizeof(cl_mem), &t3_uv_sign);
	status |= clSetKernelArg(mKernels[0], 3, sizeof(unsigned int), &offset);
	status |= clSetKernelArg(mKernels[0], 4, sizeof(int), &n_t3);
	status |= clSetKernelArg(mKernels[0], 5, sizeof(cl_mem), &output);      // Output is stored on the GPU.
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.");

	// Execute the kernel over the entire range of the data set
	status = clEnqueueNDRangeKernel(mQueue, mKernels[0], 1, NULL, &global, NULL, 0, NULL, NULL);
	CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");
}

// Calculates the T3 from the Fourier transform input.
// Stores the output separated according to the COILibData.h specifications, namely T3Amp and T3Phi are separated.
void CRoutine_FTtoT3::FTtoT3(valarray<cl_float2> & ft_input, valarray<cl_uint4> & uv_ref, valarray<cl_short4> & signs, valarray<cl_float> & output)
{
	// How many T3s do we have?
	size_t n_t3 = min(uv_ref.size(), signs.size());
	output.resize(2*n_t3);

	// Locals
	cl_float2 t_ab;
	cl_float2 t_bc;
	cl_float2 t_ca;
	cl_uint4 uvpoint;
	cl_short4 sign;
	complex<float> V_ab;
	complex<float> V_bc;
	complex<float> V_ca;
	complex<float> T3;
	for(size_t i = 0; i < n_t3; i++)
	{
		// Look up the UV points
		uvpoint = uv_ref[i];
	    t_ab = ft_input[uvpoint.s[0]];
	    t_bc = ft_input[uvpoint.s[1]];
	    t_ca = ft_input[uvpoint.s[2]];

	    // Look up the signs, conjugate when necessary.
	    sign = signs[i];
	    t_ab.s[1] *= sign.s[0];
	    t_bc.s[1] *= sign.s[1];
	    t_ca.s[1] *= sign.s[2];

	    // Form complex numbers, carry out the multiplication.
		V_ab = complex<float>(t_ab.s[0], t_ab.s[1]);
		V_bc = complex<float>(t_bc.s[0], t_bc.s[1]);

		// V_ca needs to be conjugated, per the bispectra definition:
		V_ca = complex<float>(t_ca.s[0], -1*t_ca.s[1]);
		T3 = V_ab * V_bc * V_ca;

		// Assign values to the output (following the specification in COILibData.h)
		output[i] = real(T3);
		output[n_t3 + i] = imag(T3);
	}
}

} /* namespace liboi */
