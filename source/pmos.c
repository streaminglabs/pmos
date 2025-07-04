/*!
 *  \file  pmos.c
 *  \brief Parametric MOS models for multi-screen systems.
 *
 *  This code implements methods and models proposed in the following publications:
 * 
 *    [1] J. Westerink and J. Roufs, “Subjective Image Quality as a Function of Viewing Distance, Resolution, and Picture Size,” SMPTE Journal, 98(2), 1989.
 *    [2] N. Barman, R. Vanam, Y. Reznik, "Generalized Westerink-Roufs Model for Predicting Quality of Scaled Video," QoMEX'22, September 5-7, 2022.
 *    [3] N. Barman, R. Vanam, Y. Reznik, "Parametric Quality Models for Multiscreen Video Systems," EUVIP'22, September 11-14, 2022.
 *
 *  \version  1.0.0
 *  \date     Jul 3, 2025
 *  \author   Yuriy A. Reznik, yreznik@streaminglabs.com
 *
 *  \copyright (c) 2025 Streaming Labs, Ltd.
 */

#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "pmos.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef min
#define min(a,b) ((a)<=(b)?(a):(b))
#endif
#ifndef max
#define max(a,b) ((a)>=(b)?(a):(b))
#endif

/****************************
 *
 * The models:
 *
 *   wr_model()         - generalized Westerink-Roufs model [2].
 *   wr_plus_psnr2mos() - WR+PSNR2MOS model [3]
 *   wr_plus_ssim2mos() - WR+SSIM2MOS model [3]
 *   wr_plus_vif2mos()  - WR+VIF2MOS model  [3]
 *   wr_plus_vmaf2mos() - WR+VMAF2MOS model [3]
 * 
 ***/

/*!
 *  \brief Generalized Westerink-Roufs model [2].
 *
 *  \param[in] phi         viewing angle [degrees]
 *  \param[in] u           angular resolution [cycles per degree]
 *  \param[in] hdr         indicator if video is hdr (1) or sdr (0)
 *  \param[in] upsampling  upsampling method (see enum upsampling_methods)
 *
 *  \return WR quality score
 */
static double wr_model(double phi, double u, int hdr, int upsampling)
{
	/* generalized WR model parameters [2]: */
	struct wr_params { double alpha, beta, gamma, delta, k, l, phi_s, u_s; };
	static struct wr_params wr_sdr = { 2.72, 145.69, 1.55, 2.12, 6.01, 2.11, 35.0, 16.93 }; /* sdr model: [2, page 4] */
	static struct wr_params wr_hdr[n_upsampling_methods] = {
		{2.72, 106.91, 1.55 * 1.08, 2.12 * 1.08, 6.01, 1.76, 35.0, 13.93}, /* bc, hdr, [2, page 5, Table III, line 3] */
		{2.72, 106.91, 1.55 * 1.08, 2.12 * 1.08, 6.01, 2.5,  35.0, 23.4},  /* nn, hdr, [2, page 5, Table III, line 2] */
		{2.72, 106.91, 1.55 * 1.08, 2.12 * 1.08, 6.01, 2.06, 35.0, 12.24}, /* sr, hdr, [2, page 5, Table III, line 4] */
	};
	struct wr_params* wr;
	double f_phi, f_u, mos;

	/* sanity checks */
	assert(phi > 0 && phi < 180);
	assert(u > 0 && u < 1000);
	assert(upsampling >= 0 && upsampling < n_upsampling_methods);
	assert(hdr >= 0);

	/* select WR model: */
	wr = &wr_sdr;
	if (hdr) wr = wr_hdr + upsampling;

	/* compute WR score [2, formulae 8]: */
	f_phi = pow(1.0 + pow(phi / wr->phi_s, -wr->k), -wr->gamma / wr->k);
	f_u = pow(1.0 + pow(u / wr->u_s, -wr->l), -wr->delta / wr->l);
	mos = log(wr->alpha + wr->beta * f_phi * f_u);

	/* clamp it to 1..5 range: */
	mos = max(1, min(5, mos));
	return mos;
}

/*!
 *  \brief WR+PSNR2MOS model [3].
 *
 *  \param[in] phi          viewing angle [degrees]
 *  \param[in] u            angular resolution [cycles per degree]
 *  \param[in] hdr          indicator if video is hdr (1) or sdr (0)
 *  \param[in] upsampling   assumed upsampling method (see enum upsampling_methods)
 *  \param[in] psnr         PSNR full-reference score
 *
 *  \return MOS score
 */
static double wr_plus_psnr2mos(double phi, double u, int hdr, int upsampling, double psnr)
{
	/* model parameters, cf. Table 4 [3] */
	double alpha = -6.906, beta = 6.130, gamma = -0.048, delta = 1.476, epsilon = 0.228, zeta = 23.83;
	double Qpsnr, Qwr, mos;

	/* sanity checks */
	assert(phi > 0 && phi < 180);
	assert(u > 0 && u < 1000);
	assert(hdr >= 0);
	assert(upsampling >= 0 && upsampling < n_upsampling_methods);
	assert(psnr > 0 && psnr < 100);

	/* compute WR metric: */
	Qwr = wr_model(phi, u, hdr, upsampling);

	/* map PSNR to MOS scale [3, formula 4]: */
	Qpsnr = 1.0 / (1.0 + exp(-epsilon * (psnr - zeta)));

	/* compute fused MOS score [3, formula 2]: */
	mos = alpha + beta * (1 + gamma * Qwr) * Qpsnr + delta * Qwr;

	/* clamp it to 1..5 range: */
	mos = max(1, min(5, mos));
	return mos;
}

/*!
 *  \brief WR+SSIM2MOS model [3].
 *
 *  \param[in] phi          viewing angle [degrees]
 *  \param[in] u            angular resolution [cycles per degree]
 *  \param[in] hdr          indicator if video is hdr (1) or sdr (0)
 *  \param[in] upsampling   assumed upsampling method (see enum upsampling_methods)
 *  \param[in] ssim         SSIM full-reference score
 *
 *  \return MOS score
 */
static double wr_plus_ssim2mos(double phi, double u, int hdr, int upsampling, double ssim)
{
	/* model parameters, cf. Table 4 [3] */
	double alpha = -7.181, beta = 7.662, gamma = -0.089, delta = 1.753, epsilon = 7.492, zeta = 0.777;
	double Qssim, Qwr, mos;

	/* sanity checks */
	assert(phi > 0 && phi < 180);
	assert(u > 0 && u < 1000);
	assert(hdr >= 0);
	assert(upsampling >= 0 && upsampling < n_upsampling_methods);
	assert(ssim > 0 && ssim <= 1.0);

	/* compute WR metric: */
	Qwr = wr_model(phi, u, hdr, upsampling);

	/* map SSIM to MOS scale [3, formula 4]: */
	Qssim = 1.0 / (1.0 + exp(-epsilon * (ssim - zeta)));

	/* compute fused MOS score [3, formula 2]: */
	mos = alpha + beta * (1 + gamma * Qwr) * Qssim + delta * Qwr;

	/* clamp it to 1..5 range: */
	mos = max(1, min(5, mos));
	return mos;
}

/*!
 *  \brief WR+VIF2MOS model [3].
 *
 *  \param[in] phi          viewing angle [degrees]
 *  \param[in] u            angular resolution [cycles per degree]
 *  \param[in] hdr          indicator if video is hdr (1) or sdr (0)
 *  \param[in] upsampling   assumed upsampling method (see enum upsampling_methods)
 *  \param[in] vif          VIF full-reference score
 *
 *  \return MOS score
 */
static double wr_plus_vif2mos(double phi, double u, int hdr, int upsampling, double vif)
{
	/* model parameters, cf. Table 4 [3] */
	double alpha = -12.09, beta = 12.117, gamma = -0.137, delta = 2.763, epsilon = 4.846, zeta = 0.416;
	double Qvif, Qwr, mos;

	/* sanity checks */
	assert(phi > 0 && phi < 180);
	assert(u > 0 && u < 1000);
	assert(hdr >= 0);
	assert(upsampling >= 0 && upsampling < n_upsampling_methods);
	assert(vif > 0 && vif <= 1.0);

	/* compute WR metric: */
	Qwr = wr_model(phi, u, hdr, upsampling);

	/* map VIF to MOS scale [3, formula 4]: */
	Qvif = 1.0 / (1.0 + exp(-epsilon * (vif - zeta)));

	/* compute fused MOS score [3, formula 2]: */
	mos = alpha + beta * (1 + gamma * Qwr) * Qvif + delta * Qwr;

	/* clamp it to 1..5 range: */
	mos = max(1, min(5, mos));
	return mos;
}

/*!
 *  \brief WR+VMAF2MOS model [3].
 *
 *  \param[in] phi          viewing angle [degrees]
 *  \param[in] u            angular resolution [cycles per degree]
 *  \param[in] hdr          indicator if video is hdr (1) or sdr (0)
 *  \param[in] upsampling   assumed upsampling method (see enum upsampling_methods)
 *  \param[in] vmaf         VMAF full-reference score
 *
 *  \return MOS score
 */
static double wr_plus_vmaf2mos(double phi, double u, int hdr, int upsampling, double vmaf)
{
	/* model parameters, cf. Table 4 [3] */
	double alpha = -7.682, beta = 0.0753, gamma = -0.122, delta = 2.01;
	double Qvmaf, Qwr, mos;

	/* sanity checks */
	assert(phi > 0 && phi < 180);
	assert(u > 0 && u < 1000);
	assert(hdr >= 0);
	assert(upsampling >= 0 && upsampling < n_upsampling_methods);
	assert(vmaf > 0 && vmaf <= 100);

	/* compute WR metric: */
	Qwr = wr_model(phi, u, hdr, upsampling);

	/* map VMAF to MOS scale [3, formula 5]: */
	Qvmaf = vmaf;

	/* compute final MOS score [3, formula 2]: */
	mos = alpha + beta * (1 + gamma * Qwr) * Qvmaf + delta * Qwr;

	/* clamp it to 1..5 range: */
	mos = max(1, min(5, mos));
	return mos;
}

/****************************
 *
 * Functions computing parameters of viewing setup:
 * 
 *   viewing_angle()             - computes viewing angle
 *   angular_resolution()        - computes angular resolution
 *   heights_to_inches()         - translates relative viewing distance into absolute metrics
 *   device_to_viewing_params()  - computes viewing angle and angular resolution as specific to a given player and device
 * 
 ***/

/*!
 *  \brief Compute horisontal viewing angle (cf. [1-3]).
 *
 *  \param[in] player_width    width of video player window [pixels]
 *  \param[in] distance        viewing distance [pixels]
 *  \param[in] ppi_x           display's pixel density in horisontal direction [ppi]
 *
 *  \return viewing andge [degrees]
 */
static double viewing_angle(int player_width, double distance, double ppi_x)
{
	/* sanity checks */
	assert(player_width > 0);
	assert(distance > 0);
	assert(ppi_x > 0);

	/* apply formula for viewing angle */
	return 180.0 / M_PI * 2 * atan((double)player_width / (2.0 * distance * ppi_x));
}

/*!
 *  \brief Compute angular resolution (cf. [1-3]).
 *
 *  \param[in] video_width     width of encoded video [pixels]
 *  \param[in] player_width    width of a video player window [pixels]
 *  \param[in] distance        viewing distance [pixels]
 *  \param[in] ppi_x           display's pixel density in horisontal direction [ppi]
 *
 *  \return !0 = angular resolution [cycles per degree], 0 = parameter error
 */
double angular_resolution(int video_width, int player_width, double distance, double ppi_x)
{
	double width, viewing_angle_of_a_cycle;

	/* sanity checks */
	assert(video_width > 0);
	assert(player_width > 0);
	assert(distance > 0);
	assert(ppi_x > 0);

	/* effective resolution as rendered */
	width = min(video_width, player_width);

	/* apply formula for viewing angle of a cycle (2 pixels): */
	viewing_angle_of_a_cycle = 180.0 / M_PI * 2 * atan((double)player_width / ((double)width * distance * ppi_x));

	/* map to cpd: */
	return 1. / viewing_angle_of_a_cycle;
}

/*!
 *  \brief Relative viewing distance to absolute viewing distance conversion.
 *
 *  \param[in] display_height        display height [pixels]
 *  \param[in] ppi_y                 display's pixel density in vertical direction [ppi]
 *  \param[in] distance_in_heights   viewing distance [display heights]
 *
 *  \return viewing distande [inches]
 */
static double heights_to_inches(int display_height, double ppi_y, double distance_in_heights)
{
	double display_height_in_inches;
	double distance;

	/* sanity checks */
	assert(display_height > 0);
	assert(ppi_y > 0);
	assert(distance_in_heights > 0);

	/* compute distance in inches */
	display_height_in_inches = (double)display_height / ppi_y;
	distance = display_height_in_inches * distance_in_heights;

	return distance;
}

/*!
 *  Ballpark parameters for each device type (the order of records in this table follows values of enum device_types):
 */
static struct device_params devices[n_device_types] =
{
	/* w,  h,    ppi_x, ppi_y, dt, dist */
	{2400, 1080, 421,   421,   0,  13},  /* mobile  e.g. Samsung Galaxy S21, 6.2", viewing distance ~ 13"            */
	{2800, 1752, 266,   266,   0,  18},  /* tablet  e.g. Samsung Galaxy Tab S8 Plus, 12.4", viewing distance ~ 18"   */
	{2560, 1600, 100,   100,   0,  24},  /* desktop e.g. Dell UltraSharp U3011, 30", viewing distance ~ 24"          */
	{3840, 2160, 80,    80,    1,  3},   /* TV      e.g. 55" UHDTV set, viewing distance ~ 3H (81")                  */
	{0,    0,    0,     0,     0,  0}    /* custome device type - parameters to be provided by an external structure */
};

/*!
 * \brief Computes viewing angle and angular resolution as specific to a given player and device
 *  
 *  \param[in]  width			video width [pixels]
 *  \param[in]  height			video height [pixels]
 *  \param[in]  player_width	player video width [pixels]
 *  \param[in]  player_height	player video height [pixels]
 *  \param[in]	hdr				indicator if video is hdr (1) or sdr (0)
 *  \param[in]	upsampling		assumed upsampling method (see enum upsampling_methods)
 *  \param[in]  device			device type [device_type]
 *  \param[in]  params			custom device parameters
 *  \param[out] p_phi			effective viewing angle [degrees]
 *  \param[out] p_u				effective angular resolution [cycles per degree]
 *
 *  \returns    0				success
 *				-1				invalid resolution
 *				-2				invalid player size
 *				-3				invalid HDR/SDR indicator
 *				-4				invalid upsampling method
 *				-5				invalid device type
 *				-6				NULL pointer
 *				-7				invalid custom device parameters
 *				-8              internal error
 */
int device_to_viewing_params(int width, int height,int player_width, int player_height, int hdr, int upsampling, int device, struct device_params* params, double *p_phi, double *p_u)
{
	double distance, phi, u;
	struct device_params* p;

	/* check parameters */
	if (width < 1 || width > 8192) return -1;
	if (height < 1 || height > 8192) return -1;
	if (player_width < 1 || player_width > 8192) return -2;
	if (player_height < 1 || player_height > 8192) return -2;
	if (hdr < 0 || hdr > 1) return -3;
	if (upsampling < 0 || upsampling >= n_upsampling_methods) return -4;
	if (device < 0 || device >= n_device_types) return -5;
	if (p_phi == NULL) return -6;
	if (p_u == NULL) return -6;

	/* standard device? */
	if (device <= device_custom) {
		/* select default parameters for a given device: */
		p = & devices[device];
	} else {
		/* check if custom device parameter structure is present: */
		p = params;
		if (p == NULL) return -6;
		if (p->display_width < 128 || p->display_width > 16384) return -7;
		if (p->display_height < 128 || p->display_height > 16384) return -7;
		if (p->ppi_x < 1 || p->ppi_x > 10000) return -7;
		if (p->ppi_y < 1 || p->ppi_y > 10000) return -7;
		if (p->distance_type < 0) return -7;
		if (p->distance <= 0 || p->distance > 10000) return -7;
	}

	/* check if device comes with relative viewing distance: */
	distance = p->distance;
	if (p->distance_type) {
		/* compute absolute distance: */
		distance = heights_to_inches(p->display_height, p->ppi_y, p->distance);
	}

	/* compute effective viewing andgle and angular resolution: */
	phi = viewing_angle(player_width, distance, p->ppi_x);
	u = angular_resolution(width, player_width, distance, p->ppi_x);

	/* check if the results make sense: */
	if (phi < 1 || phi > 180) return -8;
	if (u < 1 || u > 200) return -8;

	/* store the results: */
	*p_phi = phi;
	*p_u = u;

	/* success */
	return 0;
}

/****************************
 *
 * External functions:
 * 
 *   psnr2mos() - maps PSNR + device parameters to MOS scores
 *   psnr2mos() - maps SSIM + device parameters to MOS scores
 *   vif2mos()  - maps VIF + device parameters to MOS scores
 *   vmaf2mos() - maps VMAF + device parameters to MOS scores
 *
 ***/

 /*!
  * \brief PSNR to device-specic MOS score mapping.
  *
  * \param[in]  psnr			PSNR score
  * \param[in]  width			video width [pixels]
  * \param[in]  height			video height [pixels]
  * \param[in]  player_width	player video width [pixels]
  * \param[in]  player_height	player video height [pixels]
  * \param[in]	hdr				indicator if video is hdr (1) or sdr (0)
  * \param[in]	upsampling		assumed upsampling method (see enum upsampling_methods)
  * \param[in]  device			device type [device_type]
  * \param[in]  params			custom device parameters
  *
  * \returns   >0   - computed MOS score (in [1..5])
  *            <0	- error
  */
double psnr2mos(double psnr, int width, int height, int player_width, int player_height, int hdr, int upsampling, int device, struct device_params* params)
{
	double phi = 0, u = 0, mos;
	int err;

	/* check input variables and compute angular parameters of viewing setup: */
	err = device_to_viewing_params(width, height, player_width, player_height, hdr, upsampling, device, params, &phi, &u);
	if (err)
		return (double)err;

	/* check if PSNR score is valid */
	if (psnr < 0 || psnr > 100)
		return -9;

	/* compute WR+PSNR2MOS quality score: */
	mos = wr_plus_psnr2mos(phi, u, hdr, upsampling, psnr);
	return mos;
}

/*!
 * \brief SSIM to device-specic MOS score mapping.
 */
double ssim2mos(double ssim, int width, int height, int player_width, int player_height, int hdr, int upsampling, int device, struct device_params* params)
{
	double phi = 0, u = 0, mos;
	int err;

	/* check input variables and compute angular parameters of viewing setup: */
	err = device_to_viewing_params(width, height, player_width, player_height, hdr, upsampling, device, params, &phi, &u);
	if (err)
		return (double)err;

	/* check if SSIM score is valid */
	if (ssim < 0 || ssim > 1.0)
		return -9;

	/* compute WR+SSIM2MOS quality score: */
	mos = wr_plus_ssim2mos(phi, u, hdr, upsampling, ssim);
	return mos;
}

/*!
 * \brief VIF to device-specic MOS score mapping.
 */
double vif2mos(double vif, int width, int height, int player_width, int player_height, int hdr, int upsampling, int device, struct device_params* params)
{
	double phi = 0, u = 0, mos;
	int err;

	/* check input variables and compute angular parameters of viewing setup: */
	err = device_to_viewing_params(width, height, player_width, player_height, hdr, upsampling, device, params, &phi, &u);
	if (err)
		return (double)err;

	/* check if VIF score is valid */
	if (vif < 0 || vif > 1.0)
		return -9;

	/* compute WR+SSIM2MOS quality score: */
	mos = wr_plus_vif2mos(phi, u, hdr, upsampling, vif);
	return mos;
}

/*!
 * \brief VMAF to device-specic MOS score mapping.
 */
double vmaf2mos(double vmaf, int width, int height, int player_width, int player_height, int hdr, int upsampling, int device, struct device_params* params)
{
	double phi = 0, u = 0, mos;
	int err;

	/* check input variables and compute angular parameters of viewing setup: */
	err = device_to_viewing_params(width, height, player_width, player_height, hdr, upsampling, device, params, &phi, &u);
	if (err)
		return (double)err;

	/* check if VIF score is valid */
	if (vmaf < 0 || vmaf > 1.0)
		return -9;

	/* compute WR+SSIM2MOS quality score: */
	mos = wr_plus_vmaf2mos(phi, u, hdr, upsampling, vmaf);
	return mos;
}

/* pmos.c -- end of file */
