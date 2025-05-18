#ifndef DEBUGWARNING_H

#define DEBUGWARNING_H

/**
 * \file debugwarning.h
 *
 * This file introduces a define called DEBUGWARNING which can be used to log something only once.
 * For instance, this can be helpful to log something only one time from the
 * constructor of a class which is instantiated often.
 */

#define DEBUGWARNING(x) \
{\
	static bool showedWarning = false; \
	if (!showedWarning) \
	{\
		showedWarning = true;\
		std::cerr << "# DEBUG WARNING: " << (x) << std::endl;\
	}\
}\

#endif // DEBUGWARNING_H
