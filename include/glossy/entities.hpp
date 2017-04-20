#ifndef glossy_entities_hpp_included
#define glossy_entities_hpp_included

#include <glossy/util.hpp>
#include <ostream>

namespace glossy {
	struct material {
		vec3 color{ 1.0, 0.0, 1.0 };
		bool checkered = false;
		bool diffuse = true;
		bool specular = false;

		std::ostream& print( std::ostream& stream ) const;
	};

	struct object {
		vec3 position{ 0.0, 0.0, 0.0 };
		material material;

		virtual ~object() = default;
		virtual std::ostream& print( std::ostream& stream ) const = 0;
		virtual char const* type() const = 0;
	};
	struct sphere : public object {
		float radius = 1.0;

		virtual std::ostream& print( std::ostream& stream ) const override;
		virtual char const* type() const override;
	};
	struct plane : public object {
		vec3 normal{ 0.0, 1.0, 0.0 };

		virtual std::ostream& print( std::ostream& stream ) const override;
		virtual char const* type() const override;
	};

	// in order to allow moving lights, the position is stored as a vector of strings
	struct light {
		strvec3 position{ "0.0", "0.0", "0.0" };
		vec3 color{ 1.0, 1.0, 1.0 };

		std::ostream& print( std::ostream& stream ) const;
	};
}

#endif // !glossy_entities_hpp_included
