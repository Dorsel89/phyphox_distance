#include "phyphoxBleExperiment.h"
#include <string.h>
void PhyphoxBleExperiment::Element::setLabel(const char *l){
	memset(&LABEL[0], 0, sizeof(LABEL));
	strcat(LABEL, l);
}
