#ifndef ATTRIBUTE_TABLE_H
#define ATTRIBUTE_TABLE_H
#include <vector>
#include <map>
#include <memory>
namespace data {
	class AttributeTable;
	typedef std::shared_ptr<AttributeTable> AttributeTablePtr;

	class AttributeTable {
	public:
		AttributeTable( void );
		virtual ~AttributeTable( void );

		static data::AttributeTablePtr create();

		std::map< std::string, std::vector<std::string> > map_view();

		void set( std::map< std::string, std::vector<std::string> > m );

		void insert( const std::string& name, const std::vector<std::string> values );
		void insert( const std::string& name, const std::string value );
		void insert( const std::string& name, std::vector<long> values );
		void insert( const std::string& name, const long value );

		void merge( data::AttributeTablePtr source);

		std::string get_string( std::string name ) const;
		std::vector<std::string> get_string_array( std::string name ) const;
		long get_long( std::string name ) const;
		std::vector<long> get_long_array( std::string name ) const;
		double get_double( std::string name ) const;
		std::vector<double> get_double_array( std::string name ) const;

		bool contains( std::string name ) const;

		virtual std::string qualify(const std::string& name) const;

		void display_attributes() const;

		void clear();

		size_t size() const;

		bool empty() const;
	private:
		std::map< std::string, std::vector<std::string> > table_;
		std::string prefix_;

	};
}

#endif