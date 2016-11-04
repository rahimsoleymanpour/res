#include ".\precision_timer.h"

//precision_timer class implementation:

//create, copy and destroy operations
precision_timer::precision_timer(void): _frequency(frequency()), _tick_count(0), _stopped(true) {}

precision_timer::precision_timer(const precision_timer& rhs): _frequency(rhs._frequency), _tick_count(rhs._tick_count), _stopped(rhs._stopped) {}

precision_timer::precision_timer(const double time_sec): _frequency(frequency()), _tick_count(static_cast<unsigned hyper>(time_sec * _frequency)), _stopped(true) {}

precision_timer::~precision_timer(void) {}

//timing operations
void precision_timer::start(void) {
	_stopped = false;
	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);
	_tick_count = t.QuadPart;
}

void precision_timer::restart(void) {
	if(_stopped) {
		_stopped = false;
		LARGE_INTEGER t;
		QueryPerformanceCounter(&t);
		_tick_count = t.QuadPart - _tick_count;
	}
	else {
		throw new runtime_error(ERROR_MESSAGE_001);
	}
}

void precision_timer::stop(void) {
	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);
	_stopped = true;
	unsigned hyper quadpart = t.QuadPart;
	_tick_count = quadpart - _tick_count;
}

//non-manipulating operations
bool precision_timer::operator == (const precision_timer& rhs) const {
	return _tick_count == rhs._tick_count;
}

bool precision_timer::operator != (const precision_timer& rhs) const {
	return _tick_count != rhs._tick_count;
}

//manipulating operations
precision_timer& precision_timer::operator= (const precision_timer& rhs) {
	if (&rhs == this) {
		return *this;
	}
	else {
		_tick_count = rhs._tick_count;
		_frequency = rhs._frequency;
		_stopped = rhs._stopped;
	}
	return *this;
}

precision_timer& precision_timer::operator= (const double time_sec) {
	_tick_count = static_cast<unsigned hyper>(time_sec * _frequency);	
	_stopped = true;
	return *this;
}

precision_timer& precision_timer::operator+= (const precision_timer& rhs) {
	operator= (elapsed() + rhs.elapsed());
	return *this;
}

precision_timer& precision_timer::operator-= (const precision_timer& rhs) {
	operator= (elapsed() - rhs.elapsed());
	return *this;
}

precision_timer& precision_timer::operator*= (const precision_timer& rhs) {
	operator= (elapsed() * rhs.elapsed());
	return *this;
}

precision_timer& precision_timer::operator/= (const precision_timer& rhs) {
	operator= (elapsed() / rhs.elapsed());
	return *this;
}

//operations for type conversions
double precision_timer::to_double(void) const{
	return elapsed();
}

string precision_timer::to_string(void) const{
	ostringstream s;
	s << elapsed();
	return s.str();
}

//accessor methods
unsigned hyper precision_timer::frequency(void) {
	LARGE_INTEGER f;
	if(!QueryPerformanceFrequency(&f)) {
		throw new runtime_error(ERROR_MESSAGE_000);
	}
	//If your compiler has built-in support for 64-bit integers like Microsoft Visual C++, use the QuadPart member, otherwise, use the LowPart and HighPart members.
	_frequency = f.QuadPart;
	return _frequency;
}

unsigned hyper precision_timer::tick_count(void) const {
	return _tick_count;
}

double precision_timer::elapsed(void) const{
	if (_stopped) {
		double t = static_cast<double>(_tick_count);
		double f = static_cast<double>(_frequency);
		return t / f;
	}
	else {
		throw new runtime_error(ERROR_MESSAGE_001);
	}
}

//class members
const string precision_timer::ERROR_MESSAGE_000 = "Error! This hardware does not support the time stamp counter necessary for this class to function.";
const string precision_timer::ERROR_MESSAGE_001 = "Error! Timer had not previously been stopped or assigned.";

//global overloaded operators
// streams
ostream& operator << (ostream& out, const precision_timer& timer) {
	return out << timer.to_string();
}

istream& operator >> (istream& in, precision_timer& timer) {
	double time_sec;
	in >> time_sec;
	timer = time_sec;
	return in; 
}

// arithmetic
const precision_timer operator+ (const precision_timer& lhs, const precision_timer& rhs) {
	return precision_timer(lhs) += rhs;
}

const precision_timer operator- (const precision_timer& lhs, const precision_timer& rhs) {
	return precision_timer(lhs) -= rhs;
}

const precision_timer operator* (const precision_timer& lhs, const precision_timer& rhs) {
	return precision_timer(lhs) *= rhs;
}

const precision_timer operator/ (const precision_timer& lhs, const precision_timer& rhs) {
	return precision_timer(lhs) /= rhs;
}


const precision_timer operator+ (const precision_timer& lhs, const double rhs) {
	return precision_timer(lhs) += precision_timer(rhs);
}

const precision_timer operator- (const precision_timer& lhs, const double rhs) {
	return precision_timer(lhs) -= precision_timer(rhs);
}

const precision_timer operator* (const precision_timer& lhs, const double rhs) {
	return precision_timer(lhs) *= precision_timer(rhs);
}

const precision_timer operator/ (const precision_timer& lhs, const double rhs) {
	return precision_timer(lhs) /= precision_timer(rhs);
}

