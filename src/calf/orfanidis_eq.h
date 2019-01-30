/*
 * Copyright (c) 2018 Fedor Uporov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <cmath>
#include <vector>
#include <complex>
#include <limits>
#include <numeric>
#include <algorithm>
#include <functional>

namespace OrfanidisEq {

/*
 * Just version.
 */
static const char* eq_version = "0.02";

/*
 * The float type usage here could be cause the lack of precession.
 */
typedef double eq_double_t;

/*
 * Eq configuration constants.
 * The defaultEqBandPassFiltersOrder should be more then 2.
 */
static const eq_double_t defaultSampleFreqHz = 48000;
static const size_t defaultEqBandPassFiltersOrder = 4;

/* Default frequency values to get frequency grid. */
static const eq_double_t lowestGridCenterFreqHz = 31.25;
static const eq_double_t bandsGridCenterFreqHz = 1000;
static const eq_double_t lowestAudioFreqHz = 20;
static const eq_double_t highestAudioFreqHz = 20000;

/* Default gain values for every type of filter channel. */
static const eq_double_t eqGainRangeDb = 40;
static const eq_double_t eqGainStepDb = 1;
static const eq_double_t eqDefaultGainDb = 0;

/*
 * Allow to convert values between different representations.
 * Also, fast conversions between linear and logarithmic scales are included.
 */
class Conversions {
	std::vector<eq_double_t> linGains;

	int linGainsIndex(eq_double_t x)
	{
		int inTx = (int)x;
		int range = linGains.size() / 2;

		if ((x >= -range) && (x < range - 1))
			return range + inTx;

		return range;
	}

	Conversions();
	Conversions(const Conversions&);
	Conversions& operator= (const Conversions&);

public:
	Conversions(int range)
	{
		int step = -range;

		while (step <= range)
			linGains.push_back(db2Lin(step++));
	}

	eq_double_t fastDb2Lin(eq_double_t x)
	{
		int intPart = x;
		eq_double_t fractPart = x - intPart;

		return linGains.at(linGainsIndex(intPart)) * (1-fractPart) +
		    linGains.at(linGainsIndex(intPart + 1)) * fractPart;
	}

	eq_double_t fastLin2Db(eq_double_t x)
	{
		if ((x >= linGains[0]) && (x < linGains[linGains.size() - 1]))
			for (size_t i = 0; i < linGains.size() - 2; i++)
				if ((x >= linGains[i]) && (x < linGains[i + 1]))
					return i - linGains.size() / 2.0 + (x - (int)(x));

		return 0;
	}

	static eq_double_t db2Lin(eq_double_t x)
	{
		return pow(10, x/20.0);
	}

	static eq_double_t lin2Db(eq_double_t x)
	{
		return 20.0*log10(x);
	}

	static eq_double_t rad2Hz(eq_double_t x, eq_double_t fs)
	{
		return 2.0*M_PI/x*fs;
	}

	static eq_double_t hz2Rad(eq_double_t x, eq_double_t fs)
	{
		return 2.0*M_PI*x/fs;
	}
};

/*
 * Filter frequency band representation.
 */
class Band {
public:
	eq_double_t minFreq;
	eq_double_t centerFreq;
	eq_double_t maxFreq;

	Band() : minFreq(0), centerFreq(0), maxFreq(0) {}
	Band(eq_double_t fl, eq_double_t fc, eq_double_t fh)
		: minFreq(fl), centerFreq(fc), maxFreq(fh) {}
};

/* Basic eq errors handling. */
typedef enum {
	no_error,
	invalid_input_data_error,
	processing_error
} eq_error_t;

/*
 * Frequency grid representation.
 * Allow to calculate all required bandpass filters frequencies
 * for equalizer construction.
 */
class FrequencyGrid {
	std::vector<Band> freqs;

public:
	FrequencyGrid() {}

	eq_error_t setBand(eq_double_t fl, eq_double_t fc, eq_double_t fh)
	{
		freqs.clear();

		return addBand(fl, fc, fh);
	}

	eq_error_t addBand(eq_double_t fl, eq_double_t fc, eq_double_t fh)
	{
		if (fl < fc && fc < fh) {
			freqs.push_back(Band(fl, fc, fh));

			return no_error;
		}

		return invalid_input_data_error;
	}

	eq_error_t addBand(eq_double_t fc, eq_double_t df)
	{
		if (fc >= df /2 ) {
			freqs.push_back(Band(fc - df / 2, fc, fc + df / 2));

			return no_error;
		}

		return invalid_input_data_error;
	}

	eq_error_t set5Bands(eq_double_t fc = bandsGridCenterFreqHz)
	{
		freqs.clear();

		if (lowestAudioFreqHz < fc && fc < highestAudioFreqHz) {
			/* Find lowest center frequency in the band. */
			eq_double_t lowestFc = fc;

			while (lowestFc > lowestGridCenterFreqHz)
				lowestFc /= 4.0;

			if (lowestFc < lowestGridCenterFreqHz)
				lowestFc *= 4.0;

			/* Calculate frequencies. */
			eq_double_t f0 = lowestFc;
			for (size_t i = 0; i < 5 ; i++) {
				freqs.push_back(Band(f0 / 2, f0, f0 * 2));
				f0 *= 4;
			}

			return no_error;
		}

		return invalid_input_data_error;
	}

	eq_error_t set10Bands(eq_double_t fc = bandsGridCenterFreqHz)
	{
		freqs.clear();

		if (lowestAudioFreqHz < fc && fc < highestAudioFreqHz) {
			/* Find lowest center frequency in the band. */
			eq_double_t lowestFc = fc;

			while (lowestFc > lowestGridCenterFreqHz)
				lowestFc /= 2;

			if (lowestFc < lowestGridCenterFreqHz)
				lowestFc *= 2;

			/* Calculate frequencies. */
			eq_double_t f0 = lowestFc;
			for (size_t i = 0; i < 10; i++) {
				freqs.push_back(
				    Band(f0 / pow(2, 0.5), f0, f0 * pow(2, 0.5)));

				f0 *= 2;
			}

			return no_error;
		}

		return invalid_input_data_error;
	}

	eq_error_t set20Bands(eq_double_t fc = bandsGridCenterFreqHz)
	{
		freqs.clear();

		if (lowestAudioFreqHz < fc && fc < highestAudioFreqHz) {
			/* Find lowest center frequency in the band. */
			eq_double_t lowestFc = fc;

			while (lowestFc >= lowestAudioFreqHz)
				lowestFc /= pow(2, 0.5);

			if (lowestFc < lowestAudioFreqHz)
				lowestFc *= pow(2, 0.5);

			/* Calculate frequencies. */
			eq_double_t f0 = lowestFc;
			for (size_t i = 0; i < 20; i++) {
				freqs.push_back(Band(f0 / pow(2, 0.25),
				    f0, f0 * pow(2, 0.25)));

				f0 *= pow(2, 0.5);
			}

			return no_error;
		}

		return invalid_input_data_error;
	}

	eq_error_t set30Bands(eq_double_t fc = bandsGridCenterFreqHz)
	{
		freqs.clear();

		if (lowestAudioFreqHz < fc && fc < highestAudioFreqHz) {
			/* Find lowest center frequency in the band. */
			eq_double_t lowestFc = fc;
			while (lowestFc > lowestAudioFreqHz)
				lowestFc /= pow(2.0, 1.0/3.0);

			if (lowestFc < lowestAudioFreqHz)
				lowestFc *= pow(2.0, 1.0/3.0);

			/* Calculate frequencies. */
			eq_double_t f0 = lowestFc;
			for (size_t i = 0; i < 30; i++) {
				freqs.push_back(Band(f0 / pow(2.0, 1.0/6.0),
				    f0, f0 * pow(2.0, 1.0/6.0)));

				f0 *= pow(2, 1.0/3.0);
			}

			return no_error;
		}

		return invalid_input_data_error;
	}

	size_t getNumberOfBands()
	{
		return freqs.size();
	}

	std::vector<Band> getFreqs()
	{
		return freqs;
	}

	size_t getFreq(size_t index)
	{
		if (index < freqs.size())
			return freqs[index].centerFreq;
		else
			return 0;
	}

	size_t getRoundedFreq(size_t index)
	{
		if (index < freqs.size()) {
			size_t freq = freqs[index].centerFreq;
			if (freq < 100) {
				return freq;
			} else if (freq < 1000) {
				size_t rest = freq % 10;
				if (rest < 5)
					return freq - rest;
				else
					return freq - rest + 10;
			} else if (freq < 10000) {
				size_t rest = freq % 100;
				if (rest < 50)
					return freq - rest;
				else
					return freq - rest + 100;
			} else if (freq >= 10000) {
				size_t rest = freq%1000;
				if (rest < 500)
					return freq - rest;
				else
					return freq - rest + 1000;
			}
		}

		return 0;
	}
};

/*
 * Second order biquad section representation.
 */
struct SOSection
{
	eq_double_t b0, b1, b2;
	eq_double_t a0, a1, a2;
};

/*
 * Fourth order biquad section representation.
 */
class FOSection {
protected:
	eq_double_t b0, b1, b2, b3, b4;
	eq_double_t a0, a1, a2, a3, a4;

	eq_double_t numBuf[4];
	eq_double_t denumBuf[4];

	eq_double_t df1FOProcess(eq_double_t in)
	{
		eq_double_t out = 0;

		out+= b0*in;
		out+= (b1*numBuf[0] - denumBuf[0]*a1);
		out+= (b2*numBuf[1] - denumBuf[1]*a2);
		out+= (b3*numBuf[2] - denumBuf[2]*a3);
		out+= (b4*numBuf[3] - denumBuf[3]*a4);

		numBuf[3] = numBuf[2];
		numBuf[2] = numBuf[1];
		numBuf[1] = numBuf[0];
		*numBuf = in;

		denumBuf[3] = denumBuf[2];
		denumBuf[2] = denumBuf[1];
		denumBuf[1] = denumBuf[0];
		*denumBuf = out;

		return out;
	}

public:
	FOSection() : b0(1), b1(0), b2(0), b3(0), b4(0),
	    a0(1), a1(0), a2(0), a3(0), a4(0)
	{
		memset(numBuf, 0, sizeof(numBuf));
		memset(denumBuf, 0, sizeof(denumBuf));
	}

	FOSection(std::vector<eq_double_t>& b, std::vector<eq_double_t> a)
	{
		memset(numBuf, 0, sizeof(numBuf));
		memset(denumBuf, 0, sizeof(denumBuf));

		b0 = b[0]; b1 = b[1]; b2 = b[2]; b3 = b[3]; b4 = b[4];
		a0 = a[0]; a1 = a[1]; a2 = a[2]; a3 = a[3]; a4 = a[4];
	}

	eq_double_t process(eq_double_t in)
	{
		return df1FOProcess(in);
	}
};

/*
 * Bandpass filter representation.
 */
class BPFilter {
public:
	BPFilter() {}
	virtual ~BPFilter() {}

	virtual eq_double_t process(eq_double_t in) = 0;
};

class ButterworthBPFilter : public BPFilter {
	std::vector<FOSection> sections;

	ButterworthBPFilter() {}
public:
	ButterworthBPFilter(ButterworthBPFilter& f)
	{
		this->sections = f.sections;
	}

	ButterworthBPFilter(size_t N,
	    eq_double_t w0, eq_double_t wb,
	    eq_double_t G, eq_double_t Gb)
	{
		/* Case if G == 0 : allpass. */
		if (G == 0) {
			sections.push_back(FOSection());
			return;
		}

		/* Get number of analog sections. */
		size_t r = N % 2;
		size_t L = (N - r) / 2;

		/* Convert gains to linear scale. */
		eq_double_t G0 = Conversions::db2Lin(0.0);
		G = Conversions::db2Lin(G);
		Gb = Conversions::db2Lin(Gb);

		eq_double_t e = sqrt((G*G - Gb*Gb) / (Gb*Gb - G0*G0));
		eq_double_t g = pow(G, 1.0 / N);
		eq_double_t g0 = pow(G0, 1.0 / N);
		eq_double_t beta = pow(e, -1.0 / N) * tan(wb / 2.0);
		eq_double_t c0 = cos(w0);

		/* Calculate every section. */
		for (size_t i = 1; i <= L; i++) {
			eq_double_t ui = (2.0 * i - 1) / N;
			eq_double_t si = sin(M_PI * ui / 2.0);
			eq_double_t Di = beta*beta + 2*si*beta + 1;

			std::vector<eq_double_t> B;
			B.push_back((g*g*beta*beta + 2*g*g0*si*beta + g0*g0)/Di);
			B.push_back(-4*c0*(g0*g0 + g*g0*si*beta)/Di);
			B.push_back(2*(g0*g0*(1 + 2*c0*c0) - g*g*beta*beta)/Di);
			B.push_back(-4*c0*(g0*g0 - g*g0*si*beta)/Di);
			B.push_back((g*g*beta*beta - 2*g*g0*si*beta + g0*g0)/Di);

			std::vector<eq_double_t> A;
			A.push_back(1);
			A.push_back(-4*c0*(1 + si*beta)/Di);
			A.push_back(2*(1 + 2*c0*c0 - beta*beta)/Di);
			A.push_back(-4*c0*(1 - si*beta)/Di);
			A.push_back((beta*beta - 2*si*beta + 1)/Di);

			sections.push_back(FOSection(B, A));
		}
	}

	~ButterworthBPFilter() {}

	static eq_double_t computeBWGainDb(eq_double_t gain)
	{
		eq_double_t bwGain = 0;
		if (gain < -3)
			bwGain = gain + 3;
		else if (gain >= -3 && gain < 3)
			bwGain = gain / sqrt(2);
		else if (gain >= 3)
			bwGain = gain - 3;

		return bwGain;
	}

	virtual eq_double_t process(eq_double_t in)
	{
		eq_double_t p0 = in, p1 = 0;

		/* Process FO sections in serial connection. */
		for (size_t i = 0; i < sections.size(); i++) {
			p1 = sections[i].process(p0);
			p0 = p1;
		}

		return p1;
	}
};

class ChebyshevType1BPFilter : public BPFilter {
	std::vector<FOSection> sections;

	ChebyshevType1BPFilter() {}
public:
	ChebyshevType1BPFilter(size_t N,
	    eq_double_t w0, eq_double_t wb,
	    eq_double_t G, eq_double_t Gb)
	{
		/* Case if G == 0 : allpass. */
		if(G == 0) {
			sections.push_back(FOSection());
			return;
		}

		/* Get number of analog sections. */
		size_t r = N % 2;
		size_t L = (N - r) / 2;

		/* Convert gains to linear scale. */
		eq_double_t G0 = Conversions::db2Lin(0.0);
		G = Conversions::db2Lin(G);
		Gb = Conversions::db2Lin(Gb);

		eq_double_t e = sqrt((G*G - Gb*Gb) / (Gb*Gb - G0*G0));
		eq_double_t g0 = pow(G0, 1.0 / N);
		eq_double_t alfa = pow(1.0 / e + pow(1 + pow(e, -2.0), 0.5), 1.0 / N);
		eq_double_t beta = pow(G / e + Gb*pow(1 + pow(e, -2.0), 0.5),1.0 / N);
		eq_double_t a = 0.5 * (alfa - 1.0 / alfa);
		eq_double_t b = 0.5*(beta - g0*g0*(1 / beta));
		eq_double_t tetta_b = tan(wb / 2);
		eq_double_t c0 = cos(w0);

		/* Calculate every section. */
		for (size_t i = 1; i <= L; i++) {
			eq_double_t ui = (2.0*i - 1.0) / N;
			eq_double_t ci = cos(M_PI * ui / 2.0);
			eq_double_t si = sin(M_PI * ui / 2.0);
			eq_double_t Di = (a*a + ci*ci) * tetta_b * tetta_b +
			    2.0 * a * si * tetta_b + 1;

			std::vector<eq_double_t> B;
			B.push_back(((b*b + g0*g0*ci*ci)*tetta_b*tetta_b + 2*g0*b*si*tetta_b + g0*g0)/Di);
			B.push_back(-4*c0*(g0*g0 + g0*b*si*tetta_b)/Di);
			B.push_back(2*(g0*g0*(1 + 2*c0*c0) - (b*b + g0*g0*ci*ci)*tetta_b*tetta_b)/Di);
			B.push_back(-4*c0*(g0*g0 - g0*b*si*tetta_b)/Di);
			B.push_back(((b*b + g0*g0*ci*ci)*tetta_b*tetta_b - 2*g0*b*si*tetta_b + g0*g0)/Di);

			std::vector<eq_double_t> A;
			A.push_back(1);
			A.push_back(-4*c0*(1 + a*si*tetta_b)/Di);
			A.push_back(2*(1 + 2*c0*c0 - (a*a + ci*ci)*tetta_b*tetta_b)/Di);
			A.push_back(-4*c0*(1 - a*si*tetta_b)/Di);
			A.push_back(((a*a + ci*ci)*tetta_b*tetta_b - 2*a*si*tetta_b + 1)/Di);

			sections.push_back(FOSection(B, A));
		}
	}

	~ChebyshevType1BPFilter() {}

	static eq_double_t computeBWGainDb(eq_double_t gain)
	{
		eq_double_t bwGain = 0;
		if (gain < 0)
			bwGain = gain + 0.1;
		else
			bwGain = gain - 0.1;

		return bwGain;
	}

	eq_double_t process(eq_double_t in)
	{
		eq_double_t p0 = in, p1 = 0;

		/* Process FO sections in serial connection. */
		for (size_t i = 0; i < sections.size(); i++) {
			p1 = sections[i].process(p0);
			p0 = p1;
		}

		return p1;
	}
};

class ChebyshevType2BPFilter : public BPFilter {
	std::vector<FOSection> sections;

	ChebyshevType2BPFilter() {}
public:
	ChebyshevType2BPFilter(size_t N,
	    eq_double_t w0, eq_double_t wb,
	    eq_double_t G, eq_double_t Gb)
	{
		/* Case if G == 0 : allpass. */
		if (G == 0) {
			sections.push_back(FOSection());
			return;
		}

		/* Get number of analog sections. */
		size_t r = N % 2;
		size_t L = (N - r) / 2;

		/* Convert gains to linear scale. */
		eq_double_t G0 = Conversions::db2Lin(0.0);
		G = Conversions::db2Lin(G);
		Gb = Conversions::db2Lin(Gb);

		eq_double_t e = sqrt((G*G - Gb*Gb) / (Gb*Gb - G0*G0));
		eq_double_t g = pow(G, 1.0 / N);
		eq_double_t eu = pow(e + sqrt(1 + e*e), 1.0 / N);
		eq_double_t ew = pow(G0*e + Gb*sqrt(1 + e*e), 1.0 / N);
		eq_double_t a = (eu - 1.0 / eu) / 2.0;
		eq_double_t b = (ew - g*g / ew) / 2.0;
		eq_double_t tetta_b = tan(wb / 2);
		eq_double_t c0 = cos(w0);

		/* Calculate every section. */
		for (size_t i = 1; i <= L; i++) {
			eq_double_t ui = (2.0 * i - 1.0) / N;
			eq_double_t ci = cos(M_PI * ui / 2.0);
			eq_double_t si = sin(M_PI * ui / 2.0);
			eq_double_t Di = tetta_b*tetta_b + 2*a*si*tetta_b +
			    a*a + ci*ci;

			std::vector<eq_double_t> B;
			B.push_back((g*g*tetta_b*tetta_b + 2*g*b*si*tetta_b + b*b + g*g*ci*ci)/Di);
			B.push_back(-4*c0*(b*b + g*g*ci*ci + g*b*si*tetta_b)/Di);
			B.push_back(2*((b*b + g*g*ci*ci)*(1 + 2*c0*c0) - g*g*tetta_b*tetta_b)/Di);
			B.push_back(-4*c0*(b*b + g*g*ci*ci - g*b*si*tetta_b)/Di);
			B.push_back((g*g*tetta_b*tetta_b - 2*g*b*si*tetta_b + b*b + g*g*ci*ci)/Di);

			std::vector<eq_double_t> A;
			A.push_back(1);
			A.push_back(-4*c0*(a*a + ci*ci + a*si*tetta_b)/Di);
			A.push_back(2*((a*a + ci*ci)*(1 + 2*c0*c0) - tetta_b*tetta_b)/Di);
			A.push_back(-4*c0*(a*a + ci*ci - a*si*tetta_b)/Di);
			A.push_back((tetta_b*tetta_b - 2*a*si*tetta_b + a*a + ci*ci)/Di);

			sections.push_back(FOSection(B, A));
		}
	}

	~ChebyshevType2BPFilter(){}

	static eq_double_t computeBWGainDb(eq_double_t gain)
	{
		eq_double_t bwGain = 0;
		if (gain < 0)
			bwGain = -1;
		else
			bwGain = 1;

		return bwGain;
	}

	eq_double_t process(eq_double_t in)
	{
		eq_double_t p0 = in, p1 = 0;

		/* Process FO sections in serial connection. */
		for (size_t i = 0; i < sections.size(); i++) {
			p1 = sections[i].process(p0);
			p0 = p1;
		}

		return p1;
	}
};

class EllipticTypeBPFilter : public BPFilter {
private:
	/* complex -1. */
	std::complex<eq_double_t> j;
	std::vector<FOSection> sections;

	EllipticTypeBPFilter() {}

	std::complex<eq_double_t> arccos(std::complex<eq_double_t> z)
	{
		return -j*log(z + sqrt(z*z - 1.0));
	}

	/*
	 * Landen transformations of an elliptic modulus.
	 */
	std::vector<eq_double_t> landen(eq_double_t k, eq_double_t tol)
	{
		std::vector<eq_double_t> v;

		if (k == 0 || k == 1.0)
			v.push_back(k);

		if (tol < 1) {
			while (k > tol) {
				k = pow(k/(1.0 + sqrt(1.0 - k*k)), 2);
				v.push_back(k);
			}
		} else {
			eq_double_t M = tol;
			for (size_t i = 1; i <= M; i++) {
				k = pow(k/(1.0 + sqrt(1.0 - k*k)), 2);
				v.push_back(k);
			}
		}

		return v;
	}

	/*
	 * Complete elliptic integral.
	 */
	void ellipk(eq_double_t k, eq_double_t tol, eq_double_t& K, eq_double_t& Kprime)
	{
		eq_double_t kmin = 1e-6;
		eq_double_t kmax = sqrt(1 - kmin*kmin);

		if (k == 1.0) {
			K = std::numeric_limits<eq_double_t>::infinity();
		} else if (k > kmax) {
			eq_double_t kp = sqrt(1.0 - k*k);
			eq_double_t L = -log(kp / 4.0);
			K = L + (L - 1) * kp*kp / 4.0;
		} else {
			std::vector<eq_double_t> v = landen(k, tol);

			std::transform(v.begin(), v.end(), v.begin(),
			    bind2nd(std::plus<eq_double_t>(), 1.0));

			K = std::accumulate(v.begin(), v.end(),
			    1, std::multiplies<eq_double_t>()) * M_PI/2.0;
		}

		if (k == 0.0) {
			Kprime = std::numeric_limits<eq_double_t>::infinity();
		} else if (k < kmin) {
			eq_double_t L = -log(k / 4.0);
			Kprime = L + (L - 1.0) * k*k / 4.0;
		} else {
			eq_double_t kp = sqrt(1.0 - k*k);
			std::vector<eq_double_t> vp = landen(kp, tol);

			std::transform(vp.begin(), vp.end(), vp.begin(),
			    bind2nd(std::plus<eq_double_t>(), 1.0));

			Kprime = std::accumulate(vp.begin(), vp.end(),
			    1.0, std::multiplies<eq_double_t>()) * M_PI/2.0;
		}
	}

	/*
	 * Solves the degree equation in analog elliptic filter design.
	 */
	eq_double_t ellipdeg2(eq_double_t n, eq_double_t k, eq_double_t tol)
	{
		const size_t M = 7;
		eq_double_t K, Kprime;

		ellipk(k, tol, K, Kprime);
		eq_double_t q = exp(-M_PI * Kprime / K);
		eq_double_t q1 = pow(q, n);
		eq_double_t s1 = 0, s2 = 0;

		for (size_t i = 1; i <= M; i++)
		{
			s1 += pow(q1, i*(i+1));
			s2 += pow(q1, i*i);
		}

		return 4 * sqrt(q1) * pow((1.0 + s1) / (1.0 + 2 * s2), 2);
	}

	eq_double_t srem(eq_double_t x, eq_double_t y)
	{
		eq_double_t z = remainder(x, y);

		return z - y * copysign(1.0, z) * ((eq_double_t)(abs(z) > y / 2.0));
	}

	/*
	 * Inverse of cd elliptic function.
	 */
	std::complex<eq_double_t> acde(std::complex<eq_double_t> w, eq_double_t k,
	    eq_double_t tol)
	{
		std::vector<eq_double_t> v = landen(k, tol);

		for (size_t i = 0; i < v.size(); i++) {
			eq_double_t v1;
			if (i == 0)
				v1 = k;
			else
				v1 = v[i - 1];

			w = w / (1.0 + sqrt(1.0 - w*w * v1*v1)) * 2.0/(1 + v[i]);
		}

		std::complex<eq_double_t> u = 2.0 / M_PI * arccos(w);
		eq_double_t K, Kprime;
		ellipk(k ,tol, K, Kprime);

		return srem(real(u), 4) + j*srem(imag(u), 2*(Kprime/K));
	}

	/*
	 * Inverse of sn elliptic function.
	 */
	std::complex<eq_double_t> asne(std::complex<eq_double_t> w, eq_double_t k,
	    eq_double_t tol)
	{
		return 1.0 - acde(w, k, tol);
	}

	/*
	 * cd elliptic function with normalized complex argument.
	 */
	std::complex<eq_double_t> cde(std::complex<eq_double_t> u, eq_double_t k,
	    eq_double_t tol)
	{
		std::vector<eq_double_t> v = landen(k, tol);
		std::complex<eq_double_t> w = cos(u * M_PI / 2.0);

		for (int i = v.size() - 1; i >= 0; i--)
			w = (1 + v[i]) * w / (1.0 + v[i] * pow(w, 2));

		return w;
	}

	/*
	 * sn elliptic function with normalized complex argument.
	 */
	std::vector<eq_double_t> sne(const std::vector<eq_double_t> &u,
	    eq_double_t k, eq_double_t tol)
	{
		std::vector<eq_double_t> v = landen(k, tol);
		std::vector<eq_double_t> w;

		for (size_t i = 0; i < u.size(); i++)
			w.push_back(sin(u[i] * M_PI / 2.0));

		for (int i = v.size() - 1; i >= 0; i--)
			for (size_t j = 0; j < w.size(); j++)
				w[j] = ((1 + v[i])*w[j])/(1 + v[i]*w[j]*w[j]);

		return w;
	}

	/*
	 * Solves the degree equation in analog elliptic filter design.
	 */
	eq_double_t ellipdeg(size_t N, eq_double_t k1, eq_double_t tol)
	{
		eq_double_t L = floor(N / 2);
		std::vector<eq_double_t> ui;
		for (size_t i = 1; i <= L; i++)
			ui.push_back((2.0*i - 1.0) / N);

		eq_double_t kmin = 1e-6;
		if (k1 < kmin) {
			return ellipdeg2(1.0 / N, k1, tol);
		} else {
			eq_double_t kc = sqrt(1 - k1*k1);
			std::vector<eq_double_t> w = sne(ui, kc, tol);
			eq_double_t prod = std::accumulate(w.begin(), w.end(),
			    1.0, std::multiplies<eq_double_t>());
			eq_double_t kp = pow(kc, N) * pow(prod, 4);

			return sqrt(1 - kp*kp);
		}
	}

	/*
	 * Bilinear transformation of analog second-order sections.
	 */
	void blt(const std::vector<SOSection>& aSections, eq_double_t w0,
	    std::vector<FOSection>& sections)
	{
		eq_double_t c0 = cos(w0);
		size_t K = aSections.size();

		std::vector<std::vector<eq_double_t> > B, A, Bhat, Ahat;
		for (size_t i = 0; i < K; i++) {
			B.push_back(std::vector<eq_double_t>(5));
			A.push_back(std::vector<eq_double_t>(5));
			Bhat.push_back(std::vector<eq_double_t>(3));
			Ahat.push_back(std::vector<eq_double_t>(3));
		}

		std::vector<eq_double_t> B0(3), B1(3), B2(3), A0(3), A1(3), A2(3);
		B0[0] = aSections[0].b0; B0[1] = aSections[1].b0; B0[2] = aSections[2].b0;
		B1[0] = aSections[0].b1; B1[1] = aSections[1].b1; B1[2] = aSections[2].b1;
		B2[0] = aSections[0].b2; B2[1] = aSections[1].b2; B2[2] = aSections[2].b2;
		A0[0] = aSections[0].a0; A0[1] = aSections[1].a0; A0[2] = aSections[2].a0;
		A1[0] = aSections[0].a1; A1[1] = aSections[1].a1; A1[2] = aSections[2].a1;
		A2[0] = aSections[0].a2; A2[1] = aSections[1].a2; A2[2] = aSections[2].a2;

		/* Find 0th-order sections (i.e., gain sections). */
		std::vector<size_t> zths;
		for (size_t i = 0; i < B0.size(); i++)
			if ((B1[i] == 0 && A1[i] == 0) && (B2[i] == 0 && A2[i] == 0))
				zths.push_back(i);

		for (size_t i = 0; i < zths.size(); i++) {
			size_t j = zths[i];
			Bhat[j][0] = B0[j] / A0[j];
			Ahat[j][0] = 1;
			B[j][0] = Bhat[j][0];
			A[j][0] = 1;
		}

		/* Find 1st-order analog sections. */
		std::vector<size_t> fths;
		for (size_t i = 0; i < B0.size(); i++)
			if ((B1[i] != 0 || A1[i] != 0) && (B2[i] == 0 && A2[i] == 0))
				fths.push_back(i);

		for (size_t i = 0; i < fths.size(); i++) {
			size_t j = fths[i];
			eq_double_t D = A0[j] + A1[j];
			Bhat[j][0] = (B0[j] + B1[j]) / D;
			Bhat[j][1] = (B0[j] - B1[j]) / D;
			Ahat[j][0] = 1;
			Ahat[j][1] = (A0[j] - A1[j]) / D;

			B[j][0] = Bhat[j][0];
			B[j][1] = c0 * (Bhat[j][1] - Bhat[j][0]);
			B[j][2] = -Bhat[j][1];
			A[j][0] = 1;
			A[j][1] = c0 * (Ahat[j][1] - 1);
			A[j][2] = -Ahat[j][1];
		}

		/* Find 2nd-order sections. */
		std::vector<size_t> sths;
		for (size_t i = 0; i < B0.size(); i++)
			if (B2[i] != 0 || A2[i] != 0)
				sths.push_back(i);

		for (size_t i = 0; i < sths.size(); i++) {
			size_t j = sths[i];
			eq_double_t D = A0[j] + A1[j] + A2[j];
			Bhat[j][0] = (B0[j] + B1[j] + B2[j]) / D;
			Bhat[j][1] = 2 * (B0[j] - B2[j]) / D;
			Bhat[j][2] = (B0[j] - B1[j] + B2[j]) / D;
			Ahat[j][0] = 1;
			Ahat[j][1] = 2 * (A0[j] - A2[j]) / D;
			Ahat[j][2] = (A0[j] - A1[j] + A2[j]) /D;

			B[j][0] = Bhat[j][0];
			B[j][1] = c0 * (Bhat[j][1] - 2 * Bhat[j][0]);
			B[j][2] = (Bhat[j][0] - Bhat[j][1] + Bhat[j][2]) *c0*c0 - Bhat[j][1];
			B[j][3] = c0 * (Bhat[j][1] - 2 * Bhat[j][2]);
			B[j][4] = Bhat[j][2];

			A[j][0] = 1;
			A[j][1] = c0 * (Ahat[j][1] - 2);
			A[j][2] = (1 - Ahat[j][1] + Ahat[j][2])*c0*c0 - Ahat[j][1];
			A[j][3] = c0 * (Ahat[j][1] - 2*Ahat[j][2]);
			A[j][4] = Ahat[j][2];
		}

		/* LP or HP shelving filter. */
		if (c0 == 1 || c0 == -1) {
			for (size_t i = 0; i < Bhat.size(); i++) {
				B[i] = Bhat[i];
				A[i] = Ahat[i];
			}

			for (size_t i = 0; i < B.size(); i++) {
				B[i][1] *= c0;
				A[i][1] *= c0;

				B[i][3] = 0; B[i][4] = 0;
				A[i][3] = 0; A[i][4] = 0;
			}
		}

		for (size_t i = 0; i < B.size(); i++)
			sections.push_back(FOSection(B[i], A[i]));
	}

public:
	EllipticTypeBPFilter(size_t N,
	    eq_double_t w0, eq_double_t wb,
	    eq_double_t G, eq_double_t Gb) :
	    j(std::complex<long double>(0.0L, 1.0L))
	{
		/* Case if G == 0 : allpass. */
		if(G == 0) {
			sections.push_back(FOSection());
			return;
		}

		const eq_double_t tol = 2.2e-16;
		eq_double_t Gs =  G - Gb;

		/* Get number of analog sections. */
		size_t r = N % 2;
		size_t L = (N - r) / 2;

		/* Convert gains to linear scale. */
		eq_double_t G0 = Conversions::db2Lin(0.0);
		G = Conversions::db2Lin(G);
		Gb = Conversions::db2Lin(Gb);
		Gs = Conversions::db2Lin(Gs);

		eq_double_t WB = tan(wb / 2.0);
		eq_double_t e = sqrt((G*G - Gb*Gb) / (Gb*Gb - G0*G0));
		eq_double_t es = sqrt((G*G - Gs*Gs) / (Gs*Gs - G0*G0));
		eq_double_t k1 = e / es;
		eq_double_t k = ellipdeg(N, k1, tol);
		std::complex<eq_double_t> ju0 = asne(j*G / e / G0, k1, tol) / (eq_double_t)N;
		std::complex<eq_double_t> jv0 = asne(j / e, k1, tol) / (eq_double_t)N;

		/* Initial initialization of analog sections. */
		std::vector<SOSection> aSections;
		if (r == 0) {
			SOSection ba = {Gb, 0, 0, 1, 0, 0};
			aSections.push_back(ba);
		} else if (r == 1) {
			eq_double_t A00, A01, B00, B01;
			if (G0 == 0.0 && G != 0.0) {
				B00 = G * WB;
				B01 = 0.0;
			} else {
				eq_double_t z0 = std::real(j* cde(-1.0 + ju0, k, tol));
				B00 = G * WB;
				B01 = -G / z0;
			}
			A00 = WB;
			A01 = -1 / std::real(j * cde(-1.0 + jv0, k, tol));
			SOSection ba = {B00, B01, 0, A00, A01, 0};
			aSections.push_back(ba);
		}

		if (L > 0) {
			for (size_t i = 1; i <= L; i++) {
				eq_double_t ui = (2.0 * i - 1) / N;
				std::complex<eq_double_t> poles, zeros;

				if (G0 == 0.0 && G != 0.0)
					zeros = j / (k * cde(ui, k, tol));
				else if (G0 != 0.0 && G == 0.0)
					zeros = j * cde(ui, k, tol);
				else
					zeros = j * cde(ui - ju0, k ,tol);

				poles = j * cde(ui - jv0, k, tol);

				SOSection sa = {
				    WB*WB, -2*WB*std::real(1.0/zeros), pow(abs(1.0/zeros), 2),
				    WB*WB, -2*WB*std::real(1.0/poles), pow(abs(1.0/poles), 2)};

				aSections.push_back(sa);
			}
		}

		blt(aSections, w0, sections);

	}

	~EllipticTypeBPFilter(){}

	static eq_double_t computeBWGainDb(eq_double_t gain)
	{
		eq_double_t bwGain = 0;
		if (gain < 0)
			bwGain = gain + 0.05;
		else
			bwGain = gain - 0.05;

		return bwGain;
	}

	eq_double_t process(eq_double_t in)
	{
		eq_double_t p0 = in, p1 = 0;

		/* Process FO sections in serial connection. */
		for (size_t i = 0; i < sections.size(); i++) {
			p1 = sections[i].process(p0);
			p0 = p1;
		}

		return p1;
	}
};

/*
 * The next filter types are supported.
 */
typedef enum {
	none,
	butterworth,
	chebyshev1,
	chebyshev2,
	elliptic
} filter_type;

/*
 * Representation of single precomputed equalizer channel
 * contain vector of filters for every band gain value.
 */
class EqChannel {
	eq_double_t f0;
	eq_double_t fb;
	eq_double_t samplingFrequency;
	eq_double_t gainRangeDb;
	eq_double_t gainStepDb;

	size_t currentFilterIndex;
	eq_double_t currentGainDb;

	std::vector<BPFilter*> filters;
	filter_type currentChannelType;

	EqChannel() {}

	size_t getFltIndex(eq_double_t gainDb)
	{
		size_t numberOfFilters = filters.size();
		eq_double_t scaleCoef = gainDb / gainRangeDb;

		return (numberOfFilters / 2) + (numberOfFilters / 2) * scaleCoef;
	}

	void cleanupFiltersArray()
	{
		for(size_t j = 0; j < filters.size(); j++)
			delete filters[j];
	}

public:
	EqChannel(filter_type ft,
	    eq_double_t fs, eq_double_t f0, eq_double_t fb,
	    eq_double_t gainRangeDb = eqGainRangeDb,
	    eq_double_t gainStepDb = eqGainStepDb)
	{
		samplingFrequency = fs;
		this->f0 = f0;
		this->fb = fb;
		this->gainRangeDb = gainRangeDb;
		this->gainStepDb = gainStepDb;
		currentGainDb = 0;
		currentFilterIndex = 0;
		currentChannelType = ft;

		setChannel(currentChannelType, samplingFrequency);
	}

	~EqChannel()
	{
		cleanupFiltersArray();
	}

	eq_error_t setChannel(filter_type ft, eq_double_t fs)
	{
		(void)fs;

		eq_double_t wb = Conversions::hz2Rad(fb, samplingFrequency);
		eq_double_t w0 = Conversions::hz2Rad(f0, samplingFrequency);

		for (eq_double_t gain = -gainRangeDb; gain <= gainRangeDb;
		    gain+= gainStepDb) {
			switch(ft) {
			case (butterworth): {
				eq_double_t bw_gain =
				    ButterworthBPFilter::computeBWGainDb(gain);

				ButterworthBPFilter* bf =
				    new ButterworthBPFilter(
				    defaultEqBandPassFiltersOrder, w0,
				    wb, gain, bw_gain);

				filters.push_back(bf);
				break;
			}

			case (chebyshev1): {
				eq_double_t bwGain =
				    ChebyshevType1BPFilter::computeBWGainDb(gain);

				ChebyshevType1BPFilter* cf1 =
				    new ChebyshevType1BPFilter(
				    defaultEqBandPassFiltersOrder, w0,
				    wb, gain, bwGain);

				filters.push_back(cf1);
				break;
			}

			case (chebyshev2): {
				eq_double_t bwGain =
				    ChebyshevType2BPFilter::computeBWGainDb(gain);

				ChebyshevType2BPFilter* cf2 =
				    new ChebyshevType2BPFilter(
				    defaultEqBandPassFiltersOrder, w0,
				    wb, gain, bwGain);

				filters.push_back(cf2);
				break;
			}

			case (elliptic): {
				eq_double_t bwGain =
				    EllipticTypeBPFilter::computeBWGainDb(gain);

				EllipticTypeBPFilter* e =
				    new EllipticTypeBPFilter(
				    defaultEqBandPassFiltersOrder, w0,
				    wb, gain, bwGain);

				filters.push_back(e);
				break;
			}

			default: {
				currentChannelType = none;
				return invalid_input_data_error;
			}
			}
		}

		/* Get current filter index. */
		currentGainDb = 0;
		currentFilterIndex = getFltIndex(currentGainDb);

		return no_error;
	}

	eq_error_t setGainDb(eq_double_t db)
	{
		if (db > -gainRangeDb && db < gainRangeDb) {
			currentGainDb = db;
			currentFilterIndex = getFltIndex(db);

			return no_error;
		}

		return invalid_input_data_error;
	}

	eq_error_t SBSProcess(eq_double_t *in, eq_double_t *out)
	{
		*out = filters[currentFilterIndex]->process(*in);

		return no_error;
	}
};

static const char *getFilterName(filter_type type)
{
	switch(type) {
	case butterworth:
		return "butterworth";
	case chebyshev1:
		return "chebyshev1";
	case chebyshev2:
		return "chebyshev2";
	case elliptic:
		return "elliptic";
	default:
		return "none";
	}
}

/*
 * Main equalizer class.
 */
class Eq {
	Conversions conv;
	eq_double_t samplingFrequency;
	FrequencyGrid freqGrid;
	std::vector<EqChannel*> channels;
	filter_type currentEqType;

	void cleanupChannelsArray()
	{
		for(size_t j = 0; j < channels.size(); j++)
			delete channels[j];
	}

public:
	Eq(FrequencyGrid &fg, filter_type eq_t) : conv(46)
	{
		samplingFrequency = defaultSampleFreqHz;
		freqGrid = fg;
		currentEqType = eq_t;
		setEq(freqGrid, currentEqType);
	}

	~Eq()
	{
		cleanupChannelsArray();
	}

	eq_error_t setEq(const FrequencyGrid& fg, filter_type ft)
	{
		cleanupChannelsArray();
		channels.clear();

		freqGrid = fg;
		currentEqType = ft;

		for (size_t i = 0; i < freqGrid.getNumberOfBands(); i++) {
			Band bFres = freqGrid.getFreqs()[i];

			EqChannel* eq_ch = new EqChannel(ft, samplingFrequency,
			    bFres.centerFreq, bFres.maxFreq - bFres.minFreq);

			channels.push_back(eq_ch);
			channels[i]->setGainDb(eqDefaultGainDb);
		}

		return no_error;
	}

	eq_error_t setEq(filter_type ft)
	{
		return setEq(freqGrid, ft);
	}

	eq_error_t setSampleRate(eq_double_t sr)
	{
		samplingFrequency = sr;

		return setEq(currentEqType);
	}

	eq_error_t changeGains(const std::vector<eq_double_t>& bandGains)
	{
		if (channels.size() == bandGains.size())
			for(size_t j = 0; j < channels.size(); j++)
				channels[j]->setGainDb(conv.fastLin2Db(bandGains[j]));
		else
			return invalid_input_data_error;

		return no_error;
	}

	eq_error_t changeGainsDb(const std::vector<eq_double_t>& bandGains)
	{
		if (channels.size() == bandGains.size())
			for(size_t j = 0; j < channels.size(); j++)
				channels[j]->setGainDb(bandGains[j]);
		else
			return invalid_input_data_error;

		return no_error;
	}

	eq_error_t changeBandGain(size_t bandNumber, eq_double_t bandGain)
	{
		if (bandNumber < channels.size())
			channels[bandNumber]->setGainDb(conv.fastLin2Db(bandGain));
		else
			return invalid_input_data_error;

		return no_error;
	}

	eq_error_t changeBandGainDb(size_t bandNumber, eq_double_t bandGain)
	{
		if (bandNumber < channels.size())
			channels[bandNumber]->setGainDb(bandGain);
		else
			return invalid_input_data_error;

		return no_error;
	}

	eq_error_t SBSProcessBand(size_t bandNumber, eq_double_t *in,
	    eq_double_t *out)
	{
		if (bandNumber < getNumberOfBands())
			channels[bandNumber]->SBSProcess(in, out);
		else
			return invalid_input_data_error;

		return no_error;
	}

	eq_error_t SBSProcess(eq_double_t *in, eq_double_t *out)
	{
		eq_error_t err = no_error;
		eq_double_t inOut = *in;

		for (size_t i = 0; i < getNumberOfBands(); i++) {
			err = SBSProcessBand(i, &inOut, &inOut);
			if (err)
				return err;
		}

		*out = inOut;

		return no_error;
	}

	filter_type getEqType()
	{
		return currentEqType;
	}

	const char* getStringEqType()
	{
		return getFilterName(currentEqType);
	}

	size_t getNumberOfBands()
	{
		return freqGrid.getNumberOfBands();
	}

	const char *getVersion()
	{
		return eq_version;
	}
};

} //namespace OrfanidisEq
