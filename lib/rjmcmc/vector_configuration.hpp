#ifndef __VECTOR_CONFIGURATION_HPP__
#define __VECTOR_CONFIGURATION_HPP__

#include "configuration.hpp"

namespace rjmcmc {

template<typename T, typename UnaryEnergy, typename BinaryEnergy, typename Accelerator=trivial_accelerator>
class vector_configuration
{
	typedef	std::vector<T> container;
	container	m_container;
	UnaryEnergy	m_unary_energy;
	BinaryEnergy	m_binary_energy;
	Accelerator	m_accelerator;
public:
	typedef T					value_type;
	typedef vector_configuration<T,UnaryEnergy, BinaryEnergy, Accelerator> self;
	typedef internal::modification<self>	        modification;
	typedef	typename container::const_iterator	const_iterator;
	typedef	typename container::iterator		iterator;

	vector_configuration(UnaryEnergy unary_energy, BinaryEnergy binary_energy, Accelerator accelerator=Accelerator()) : m_unary_energy(unary_energy), m_binary_energy(binary_energy), m_accelerator(accelerator)
	{}

	// energy
	inline double energy () const {
		return unary_energy()+binary_energy();
	}

	double unary_energy() const
	{
		double e = 0.;
		for (const_iterator it = begin(); it != end(); ++it)
			e += rjmcmc::apply_visitor(m_unary_energy, *it );
		return e;
	}

	double binary_energy() const
	{
		double e = 0.;
		for (const_iterator i = begin(); i != end(); ++i)
			for (const_iterator j = i+1; j != end(); ++j)
				e += rjmcmc::apply_visitor(m_binary_energy, *i, *j );
		return e;
	}

	// objects
	inline size_t size() const { return m_container.size(); }
	inline bool empty() const {	return m_container.empty(); }
	inline iterator begin() { return m_container.begin(); }
	inline iterator end  () { return m_container.end(); }
	inline const_iterator begin() const { return m_container.begin(); }
	inline const_iterator end  () const { return m_container.end(); }
	inline const value_type& operator[]( const_iterator v ) const { return *v; }
	inline const value_type& value( const_iterator v ) const { return *v; }
	inline double energy( const_iterator v ) const { return rjmcmc::apply_visitor(m_unary_energy,*v); }

	// evaluators

	template <typename Modification> double delta_energy(const Modification &modif) const
	{
		return delta_birth(modif)+delta_death(modif);
	}

	template <typename Modification> double delta_birth(const Modification &modif) const
	{
		double delta = 0;
		typedef typename Modification::birth_const_iterator bci;
		typedef typename Modification::death_const_iterator dci;
		bci bbeg = modif.birth_begin();
		bci bend = modif.birth_end();
		dci dbeg = modif.death_begin();
		dci dend = modif.death_end();
		for(bci it=bbeg; it!=bend; ++it) {
			delta += rjmcmc::apply_visitor(m_unary_energy,*it);
			for (const_iterator it2=begin(); it2 != end(); ++it2)
				if (std::find(dbeg,dend,it2)==dend)
					delta += rjmcmc::apply_visitor(m_binary_energy, *it, value(it2) );
			for (bci it2=it+1; it2 != bend; ++it2)
				delta += rjmcmc::apply_visitor(m_binary_energy, *it, *it2);
		}
		return delta;
	}

	template <typename Modification> double delta_death(const Modification &modif) const
	{
		double delta = 0;
		typedef typename Modification::death_const_iterator dci;
		dci dbeg = modif.death_begin();
		dci dend = modif.death_end();
		for(dci it=dbeg; it!=dend; ++it) {
			iterator v = *it;
			delta -= energy(v);
			for (const_iterator it2=begin(); it2 != end(); ++it2)
				delta -= rjmcmc::apply_visitor(m_binary_energy, value(*it), *it2 );
			for (dci it2=it+1; it2 != dend; ++it2)
				delta -= rjmcmc::apply_visitor(m_binary_energy, value(*it), value(*it2) );
		}
		return delta;
	}

	// manipulators
	template <typename Modification> void apply(const Modification &modif)
	{
		typedef typename Modification::birth_const_iterator bci;
		typedef typename Modification::death_const_iterator dci;
		dci dbeg = modif.death_begin();
		dci dend = modif.death_end();
		for(dci dit=dbeg; dit!=dend; ++dit) remove(*dit);
		bci bbeg = modif.birth_begin();
		bci bend = modif.birth_end();
		for(bci bit=bbeg; bit!=bend; ++bit) insert(*bit);
	}

	inline void clear() { m_container.clear(); }

	void insert(const value_type &obj) { 
		m_container.push_back(obj);
	}
	void remove( iterator v ) {
		std::swap(*v,m_container.back());
		m_container.pop_back();
	}

	// audit
	inline double audit_unary_energy () const { return unary_energy();}
	inline double audit_binary_energy() const { return binary_energy();}
	inline unsigned int audit_structure() const { return 0; }
};


template<typename T, typename U, typename B, typename A>
std::ostream& operator<<(std::ostream& o, const vector_configuration<T,U,B,A>& c) {
	o << "energy     : " << c.unary_energy() + c.binary_energy();
	o << " = " << c.unary_energy() << " + " << c.binary_energy() << " (Data+Prior)\n";
	o << "Nb objects : " << c.size() << "\n";
	{
		typename vector_configuration<T,U,B,A>::const_iterator it = c.begin(), end = c.end();
		for (; it != end; ++it)
			o << *it <<"\t" << c.energy(it)<<"\t" << c[it] << std::endl;
	}
	
	return o;
}

}; // namespace rjmcmc

#endif // __VECTOR_CONFIGURATION_HPP__
