/* 
 *  Software License Agreement (BSD License)
 *
 *  Copyright (c) 2012, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  File:    PartsBasedDetector.cpp
 *  Author:  Hilton Bristow
 *  Created: Jun 21, 2012
 */

#include "PartsBasedDetector.hpp"
#include <cstdio>
using namespace cv;
using namespace std;

PartsBasedDetector::PartsBasedDetector() {
	// TODO Auto-generated constructor stub

}

PartsBasedDetector::~PartsBasedDetector() {
	// TODO Auto-generated destructor stub
}

void PartsBasedDetector::detect(const Mat& im, vector<Candidate>& candidates) {

	// calculate a feature pyramid for the new image
	vector<Mat> pyramid;
	features_.pyramid(im, pyramid);
	for (int n = 0; n < pyramid.size(); ++n) printf("pyramid size: %d, %d\n", pyramid[n].rows, pyramid[n].cols/32);

	// convolve the feature pyramid with the Part experts
	// to get probability density for each Part
	vector2DMat pdf;
	features_.pdf(pyramid, parts_.filters(), pdf);
	for (int n = 0; n < pdf.size(); ++n) printf("pdf size: %d, %d\n", pdf[n][0].rows, pdf[n][0].cols);
	for (int m = 0; m < 10; ++m) {
		for (int n = 0; n < 10; ++n) {
			printf("%f ", pdf[0][0].at<float>(m,n*32));
		}
		printf("\n");
	}


	// use dynamic programming to predict the best detection candidates from the part responses
	vector4DMat Ix, Iy, Ik;
	vector2DMat rootv, rooti;
	dp_.min(parts_, pdf, Ix, Iy, Ik, rootv, rooti);

	// walk back down the tree to find the part locations
	dp_.argmin(parts_, rootv, rooti, features_.scales(), Ix, Iy, Ik, candidates);

}

/*! @brief Distribute the model parameters to the PartsBasedDetector classes
 *
 * @param model the monolithic model containing the deserialization of all model parameters
 */
void PartsBasedDetector::distributeModel(Model& model) {

	// the name of the Part detector
	name_ = model.name();

	// initialize the Feature engine
	features_ = HOGFeatures<float>(model.binsize(), model.nscales(), model.flen(), model.norient());

	// make sure the filters are of the correct precision for the Feature engine
	int nfilters = model.filters().size();
	for (int n = 0; n < nfilters; ++n) {
		model.filters()[n].convertTo(model.filters()[n], DataType<float>::type);
	}

	// print out one of the filters
	printf("Filter components: \n");
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			printf("%f ", model.filters()[0].at<float>(i,j+4));
		}
		printf("\n");
	}

	// initialize the tree of Parts
	parts_ = Parts(model.filters(), model.filtersi(), model.def(), model.defi(), model.bias(), model.biasi(),
			model.anchors(), model.biasid(), model.filterid(), model.defid(), model.parentid());

	// initialize the dynamic program
	dp_ = DynamicProgram(model.thresh());

}
