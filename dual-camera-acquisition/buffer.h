#ifndef BUFFER_H
#define BUFFER_H
#include "attribute_table.h"
namespace data {
	struct Buffer {
		unsigned short *data;
		data::AttributeTablePtr config;
	};
}
#endif