#pragma once

#include <string>
#include <sstream>

#include "Windows.h"

using namespace std;

/*!
\class precision_timer
\brief An exception protected public class to encapsualte the hardware time stamp counter with an intuitive user friendly interface.
\attention This software is released under The Academic Free License v. 2.1
\author &copy; Dr. Jeremy S. Thornton 2003-2005
\par License Agreement:
<tt>
This Academic Free License (the "License") applies to any original work of authorship (the "Original Work") whose owner (the "Licensor") has placed the following notice immediately following the copyright notice for the Original Work:

Licensed under the Academic Free License version 2.1

1) Grant of Copyright License. Licensor hereby grants You a world-wide, royalty-free, non-exclusive, perpetual, sublicenseable license to do the following:

a) to reproduce the Original Work in copies;

b) to prepare derivative works ("Derivative Works") based upon the Original Work;

c) to distribute copies of the Original Work and Derivative Works to the public;

d) to perform the Original Work publicly; and

e) to display the Original Work publicly.

2) Grant of Patent License. Licensor hereby grants You a world-wide, royalty-free, non-exclusive, perpetual, sublicenseable license, under patent claims owned or controlled by the Licensor that are embodied in the Original Work as furnished by the Licensor, to make, use, sell and offer for sale the Original Work and Derivative Works.

3) Grant of Source Code License. The term "Source Code" means the preferred form of the Original Work for making modifications to it and all available documentation describing how to modify the Original Work. Licensor hereby agrees to provide a machine-readable copy of the Source Code of the Original Work along with each copy of the Original Work that Licensor distributes. Licensor reserves the right to satisfy this obligation by placing a machine-readable copy of the Source Code in an information repository reasonably calculated to permit inexpensive and convenient access by You for as long as Licensor continues to distribute the Original Work, and by publishing the address of that information repository in a notice immediately following the copyright notice that applies to the Original Work.

4) Exclusions From License Grant. Neither the names of Licensor, nor the names of any contributors to the Original Work, nor any of their trademarks or service marks, may be used to endorse or promote products derived from this Original Work without express prior written permission of the Licensor. Nothing in this License shall be deemed to grant any rights to trademarks, copyrights, patents, trade secrets or any other intellectual property of Licensor except as expressly stated herein. No patent license is granted to make, use, sell or offer to sell embodiments of any patent claims other than the licensed claims defined in Section 2. No right is granted to the trademarks of Licensor even if such marks are included in the Original Work. Nothing in this License shall be interpreted to prohibit Licensor from licensing under different terms from this License any Original Work that Licensor otherwise would have a right to license.

5) This section intentionally omitted.

6) Attribution Rights. You must retain, in the Source Code of any Derivative Works that You create, all copyright, patent or trademark notices from the Source Code of the Original Work, as well as any notices of licensing and any descriptive text identified therein as an "Attribution Notice." You must cause the Source Code for any Derivative Works that You create to carry a prominent Attribution Notice reasonably calculated to inform recipients that You have modified the Original Work.

7) Warranty of Provenance and Disclaimer of Warranty. Licensor warrants that the copyright in and to the Original Work and the patent rights granted herein by Licensor are owned by the Licensor or are sublicensed to You under the terms of this License with the permission of the contributor(s) of those copyrights and patent rights. Except as expressly stated in the immediately proceeding sentence, the Original Work is provided under this License on an "AS IS" BASIS and WITHOUT WARRANTY, either express or implied, including, without limitation, the warranties of NON-INFRINGEMENT, MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY OF THE ORIGINAL WORK IS WITH YOU. This DISCLAIMER OF WARRANTY constitutes an essential part of this License. No license to Original Work is granted hereunder except under this disclaimer.

8) Limitation of Liability. Under no circumstances and under no legal theory, whether in tort (including negligence), contract, or otherwise, shall the Licensor be liable to any person for any direct, indirect, special, incidental, or consequential damages of any character arising as a result of this License or the use of the Original Work including, without limitation, damages for loss of goodwill, work stoppage, computer failure or malfunction, or any and all other commercial damages or losses. This limitation of liability shall not apply to liability for death or personal injury resulting from Licensor's negligence to the extent applicable law prohibits such limitation. Some jurisdictions do not allow the exclusion or limitation of incidental or consequential damages, so this exclusion and limitation may not apply to You.

9) Acceptance and Termination. If You distribute copies of the Original Work or a Derivative Work, You must make a reasonable effort under the circumstances to obtain the express assent of recipients to the terms of this License. Nothing else but this License (or another written agreement between Licensor and You) grants You permission to create Derivative Works based upon the Original Work or to exercise any of the rights granted in Section 1 herein, and any attempt to do so except under the terms of this License (or another written agreement between Licensor and You) is expressly prohibited by U.S. copyright law, the equivalent laws of other countries, and by international treaty. Therefore, by exercising any of the rights granted to You in Section 1 herein, You indicate Your acceptance of this License and all of its terms and conditions.

10) Termination for Patent Action. This License shall terminate automatically and You may no longer exercise any of the rights granted to You by this License as of the date You commence an action, including a cross-claim or counterclaim, against Licensor or any licensee alleging that the Original Work infringes a patent. This termination provision shall not apply for an action alleging patent infringement by combinations of the Original Work with other software or hardware.

11) Jurisdiction, Venue and Governing Law. Any action or suit relating to this License may be brought only in the courts of a jurisdiction wherein the Licensor resides or in which Licensor conducts its primary business, and under the laws of that jurisdiction excluding its conflict-of-law provisions. The application of the United Nations Convention on Contracts for the International Sale of Goods is expressly excluded. Any use of the Original Work outside the scope of this License or after its termination shall be subject to the requirements and penalties of the U.S. Copyright Act, 17 U.S.C. Â§ 101 et seq., the equivalent laws of other countries, and international treaty. This section shall survive the termination of this License.

12) Attorneys Fees. In any action to enforce the terms of this License or seeking damages relating thereto, the prevailing party shall be entitled to recover its costs and expenses, including, without limitation, reasonable attorneys' fees and costs incurred in connection with such action, including any appeal of such action. This section shall survive the termination of this License.

13) Miscellaneous. This License represents the complete agreement concerning the subject matter hereof. If any provision of this License is held to be unenforceable, such provision shall be reformed only to the extent necessary to make it enforceable.

14) Definition of "You" in This License. "You" throughout this License, whether in upper or lower case, means an individual or a legal entity exercising rights under, and complying with all of the terms of, this License. For legal entities, "You" includes any entity that controls, is controlled by, or is under common control with you. For purposes of this definition, "control" means (i) the power, direct or indirect, to cause the direction or management of such entity, whether by contract or otherwise, or (ii) ownership of fifty percent (50%) or more of the outstanding shares, or (iii) beneficial ownership of such entity.

15) Right to Use. You may use the Original Work in all ways not otherwise restricted or conditioned by this License or by law, and Licensor promises not to interfere with or be responsible for such uses by You.

This license is Copyright (C) 2003-2004 Lawrence E. Rosen. All rights reserved. Permission is hereby granted to copy and distribute this license without modification. This license may not be modified without the express written permission of its copyright owner.
</tt>
\version 2.0
\date 15 October 2005

The <em>hardware time stamp counter</em> or <em>hardware precision timer</em> is a hardware timer running at the CPU frequency giving a microsecond timing precision.
microsecond = (one millionth (10^-6) of a second, that is: one thousandth of a millisecond, 1 millionth(0.000001) of a second.

Overloads stream operators << and >>
Overloads mathematical operators + - * / for types precision_timer and double

\par References
http://www.codeproject.com/cpp/precisetimer.asp

*/
class precision_timer {

public:
	
//create, copy and destroy operations

	/*!
	\brief default constructor 
	construction fails in the absence of hardware support for time stamp counter
	\throws runtime_error 
	where no hardware support for time stamp counter
	*/
	precision_timer(void);

	/*!
	\brief copy constructor.
	hardware must support TSC otherwise no arguement would exist to copy
	\param precision_timer& rhs - reference to the precision_timer to be copied
	*/
	precision_timer(const precision_timer& rhs);
	
	/*!
	\brief conversion constructor
	construction fails in the absence of hardware support for TSC	
	Enables C++ to implicit type convert through the creation of temporary objects, permitting:
	\par Example
	\code
	ptimer1 = ptimer2 + 17.1;

	ptimer1 = 1000 - ptimer2;
	\endcode
	\param double time_sec - the floating point time (seconds) value to initialize this class with
	\note following the principle of least action seconds was chosen as it is the SI Unit for time (Système International d'Unités) 
	\throws runtime_error 
	where no hardware support for time stamp counter
	*/
	precision_timer(const double time_sec);

	/*!
	\note virtual destructor to inidicate that this class is available for extension through inheritance
	'always program in the future tense' Scott Meyers
	*/
	~precision_timer(void);

//timing operations

	/*!
	start recording elapsed time by taking a snapshot of the time stamp counter
	*/
	void start(void);

	/*!
	restart recording the elapsed time from where the timer last halted 
	*/
	void restart(void);

	/*!
	stop recording elapsed time by subtracting the start snapshot and adding to the elapsed time
	*/
	void stop(void);

//non-manipulating operations

	/*!
	\brief equality operator
	comparision is based ONLY on the tick count of the class, the frequency is presumed to be constant within the hardware context
	\param precision_timer& rhs - reference to the precision_timer to be compared
	\return bool - true if both tick counts are equal 
	*/
	bool operator== (const precision_timer& rhs) const;
	/*!
	\brief inequality operator
	comparision is based ONLY on the tick count of the class, the frequency is presumed to be constant within the hardware context
	\param precision_timer& rhs - reference to the precision_timer to be compared
	\return bool - true if tick counts are not equal 
	*/
	bool operator!= (const precision_timer& rhs) const;

//manipulating operations

	/*!
	assignment operator
	\param precision_timer rhs - the precision_timer to be assignment copied
	\return precision_timer& - reference to this modified timer
	*/
	precision_timer& operator= (const precision_timer& rhs);

	/*!
	assignment operator
	\param double time_sec - the floating point time (seconds) value
	\return precision_timer& - reference to this modified timer
	*/
	precision_timer& operator= (const double time_sec);

	/*!
	\brief addition operator
	\param precision_timer& rhs - the arguement to add to this timer
	\return precision_timer& - reference to this modified timer
	\throws - runtime_exception if timers have not been stopped
	*/
	precision_timer& operator+= (const precision_timer& rhs);

	/*!
	subtraction operator
	\param precision_timer& rhs - the arguement to subtract from this timer
	\return precision_timer& - reference to this modified timer
	\throws - runtime_exception if timers have not been stopped
	*/
	precision_timer& operator-= (const precision_timer& rhs);

	/*!
	multiplaction operator
	\param precision_timer& rhs - the arguement to multiply this timer
	\return precision_timer& - reference to this modified timer
	\throws - runtime_exception if timers have not been stopped
	*/
	precision_timer& operator*= (const precision_timer& rhs);

	/*!
	division operator
	\param precision_timer& rhs - the arguement to divide this timer
	\return precision_timer& - reference to this modified timer
	\throws - runtime_exception if timers have not been stopped
	*/
	precision_timer& operator/= (const precision_timer& rhs);

//operations for type conversions

	/*!
	convert this timer to a floating point representation
	\return double - time (seconds)
	*/
	double to_double(void) const;

	/*!
	convert this timer to a string representation 
	\return string - elapsed time (seconds)
	*/
	string to_string(void) const;

//accessor methods

	/*!
	establish if this hardware supports a time stamp counter and if so return the frequency
	\return unsigned hyper int - the counter frequency (Hz)
	\throws runtime_error
	*/
	unsigned hyper frequency(void); 

	/*!
	\brief return the current value of _tick_count
	\note following the principle of least action seconds was chosen as it is the SI Unit for time (Système International d'Unités) 
	\return double - elapsed time in seconds
	*/
	unsigned hyper tick_count(void) const;

	/*!
	\brief return the elapsed time in seconds
	\attention if the timer has not yet been stopped it throws a run time exception allerting this fact
	\note following the principle of least action seconds was chosen as it is the SI Unit for time (Système International d'Unités) 
	\return double - elapsed time in seconds
	\throws runtime_error
	*/
	double elapsed(void) const;   

//class members
private:

	unsigned hyper _frequency; //!< time stamp counter frequency in counts per second
	unsigned hyper _tick_count; //!< elapsed time as time stamp counts

	bool _stopped; //!< flag to trip exception if try to use elapsed time if timer not stopped!

	const static string ERROR_MESSAGE_000; //!< static class error message
	const static string ERROR_MESSAGE_001; //!< static class error message

	const static unsigned int MILLISECONDS_PER_SECOND = 1000000; //!< class constant miliseconds/seconds

};

//global overloaded operators
//streams
/*!
\brief canonical output stream operator
*/
ostream& operator << (ostream& out, const precision_timer& timer);

/*!
\brief canonical input stream operator
*/
istream& operator >> (istream& in, precision_timer& timer);

//arithmetic
/*!
\brief addition of 2 precision_timers
\param precision_timer& lhs - left hand side arguement
\param precision_timer& rhs - right hand side arguement
\return precision_timer - new precision_timer object result
\throws runtime_error - if user attempts operation on timer that has not been stopped
*/
const precision_timer operator+ (const precision_timer& lhs, const precision_timer& rhs);

/*!
\brief subtraction of 2 precision_timers
\param precision_timer& lhs - left hand side arguement
\param precision_timer& rhs - right hand side arguement
\return precision_timer - new precision_timer object result
\throws runtime_error - if user attempts operation on timer that has not been stopped
*/
const precision_timer operator- (const precision_timer& lhs, const precision_timer& rhs);

/*!
\brief multiplication of 2 precision_timers
\param precision_timer& lhs - left hand side arguement
\param precision_timer& rhs - right hand side arguement
\return precision_timer - new precision_timer object result
\throws runtime_error - if user attempts operation on timer that has not been stopped
*/
const precision_timer operator* (const precision_timer& lhs, const precision_timer& rhs);

/*!
\brief division of precision_timer by another precision_timer
\param precision_timer& lhs - left hand side arguement
\param precision_timer& rhs - right hand side arguement
\return precision_timer - new precision_timer object result
\throws runtime_error - if user attempts operation on timer that has not been stopped
*/
const precision_timer operator/ (const precision_timer& lhs, const precision_timer& rhs);


/*!
\brief addition of double to precision_timer
\param precision_timer& lhs - left hand side arguement
\param precision_timer& rhs - right hand side arguement
\return precision_timer - new precision_timer object result
\throws runtime_error - if user attempts operation on timer that has not been stopped
*/
const precision_timer operator+ (const precision_timer& lhs, const double rhs);

/*!
\brief subtraction of double from precision_timer
\param precision_timer& lhs - left hand side arguement
\param precision_timer& rhs - right hand side arguement
\return precision_timer - new precision_timer object result
\throws runtime_error - if user attempts operation on timer that has not been stopped
*/
const precision_timer operator- (const precision_timer& lhs, const double rhs);

/*!
\brief multiplication of precision_timer by double
\param precision_timer& lhs - left hand side arguement
\param precision_timer& rhs - right hand side arguement
\return precision_timer - new precision_timer object result
\throws runtime_error - if user attempts operation on timer that has not been stopped
*/
const precision_timer operator* (const precision_timer& lhs, const double rhs);

/*!
\brief division of precision_timer by double
\param precision_timer& lhs - left hand side arguement
\param precision_timer& rhs - right hand side arguement
\return precision_timer - new precision_timer object result
\throws runtime_error - if user attempts operation on timer that has not been stopped
*/
const precision_timer operator/ (const precision_timer& lhs, const double rhs);