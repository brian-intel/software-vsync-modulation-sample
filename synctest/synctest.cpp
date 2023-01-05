/*INTEL CONFIDENTIAL
Copyright (C) 2022 Intel Corporation
This software and the related documents are Intel copyrighted materials, and your use of them is governed by the express license under which they were provided to you ("License"). Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties, other than those that are expressly stated in the License.
*/
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <vsyncalter.h>
#include <debug.h>
#include <math.h>

using namespace std;

/*******************************************************************************
 * Description
 *	main - This is the main function
 * Parameters
 *	int argc - The number of command line arguments
 *	char *argv[] - Each command line argument in an array
 * Return val
 *	int - 0 = SUCCESS, 1 = FAILURE
 ******************************************************************************/
int main(int argc, char *argv[])
{
	int ret = 0;

	if(vsync_lib_init()) {
		ERR("Couldn't initialize vsync lib\n");
		return 1;
	}

	synchronize_vsync(1.0);

	vsync_lib_uninit();
	return ret;
}
