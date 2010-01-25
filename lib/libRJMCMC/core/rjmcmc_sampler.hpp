#ifndef __RJMCMC_SAMPLER_HPP__
#define __RJMCMC_SAMPLER_HPP__

#include "geometry/geometry.h"
#include "core/random.hpp"
#include "core/bbox.hpp"
#include <boost/tuple/tuple.hpp>

namespace rjmcmc {

namespace internal {

// apply a random function object within a tuple
// x should originaly be drawn uniformly in [0,1]
// t is the tuple of function objects
template<typename C, typename M>
inline unsigned int random_apply(double x, const boost::tuples::null_type&, C &c, M &m, double& R, unsigned int i=0) {
	R=0;
	return i;
}

template <class H, class T, typename C, typename M>
inline unsigned int random_apply(double x, boost::tuples::cons<H, T>& t, C &c, M &m, double& R, unsigned int i=0) {
	double p = t.get_head().probability();
	if(x<p) { R=t.get_head()(c,m); return i+t.get_head().kernel_id(); }
	return random_apply(x-p,t.get_tail(),c,m,R,i+H::size);
}

template<typename T> struct cons_list_kernel_traits {
	enum { size = 0 };
};

template <class H, class T> struct cons_list_kernel_traits < boost::tuples::cons<H, T> > {
	enum { size = H::size + cons_list_kernel_traits<T>::size };
};

}

template<typename T> struct kernel_traits {
	enum { size = internal::cons_list_kernel_traits<typename T::inherited>::size };
};


template<typename Variant> struct variant_size {
	enum { value = 1 };
};
template<BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct variant_size<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> > {
	typedef boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> T;
	enum { value = boost::mpl::size<typename T::types>::value };
};


template<typename Variant> 
inline void uniform_random_type_init(Variant &v)
{
}

#include <boost/preprocessor/iteration/local.hpp>
template<BOOST_VARIANT_ENUM_PARAMS(typename T)> 
inline void uniform_random_type_init(boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> &v)
{
	typedef boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> T;
	typedef typename T::types Types;
	typedef boost::variate_generator<RJMCMCRandom&, boost::uniform_smallint<> > die_type;
	const int N = variant_size<T>::value;
	die_type vdie(GetRandom(), boost::uniform_smallint<>(0,N-1));
	switch(vdie())        { 
#define BOOST_PP_LOCAL_LIMITS (0,BOOST_VARIANT_LIMIT_TYPES-1)
#define BOOST_PP_LOCAL_MACRO(n) case n : v = typename boost::mpl::at_c<Types,n%N>::type(); break;
#include BOOST_PP_LOCAL_ITERATE()
		default : break;
	}
}


//////////////////


template<typename Kernel0, typename Kernel1> class binary_kernel {
	double m_p[2], m_p_sum;
	Kernel0 m_kernel0;
	Kernel1 m_kernel1;
	mutable boost::variate_generator<RJMCMCRandom&, boost::uniform_real<> > m_die;
	mutable unsigned int m_kernel_id;
public:
	enum { size = 2 };
	inline unsigned int kernel_id() const { return m_kernel_id; }
	binary_kernel(const Kernel0& k0, const Kernel1& k1, double p=1.) : 
		m_kernel0(k0), m_kernel1(k1), m_p_sum(p),
		m_die(GetRandom(), boost::uniform_real<>(0,1)) {
		m_p[0]=m_p[1]=p*0.5;
	}
	binary_kernel(const Kernel0& k0, const Kernel1& k1, double p0, double p1) :
		m_kernel0(k0), m_kernel1(k1), m_p_sum(p0+p1),
		m_die(GetRandom(), boost::uniform_real<>(0,1)) {
		m_p[0]=p0; m_p[1]=p1;
	}
	inline void probability(double p_sum) { 
		double r = p_sum/m_p_sum;
		m_p[0] *= r; m_p[1] *= r;
		m_p_sum = p_sum;
	}
	inline double probability() const { return m_p_sum; }
	inline void probability(unsigned int i, double p) {
		m_p_sum += p-m_p[i];
		m_p[i] = p;
	}
	inline double probability(unsigned int i) const { return m_p[i]; }
	template<typename Configuration, typename Modification>
	double operator()(Configuration& c, Modification& modif) const
	{
		if(m_die()*m_p_sum<m_p[0]) {
			m_kernel_id = 0;
			double x = m_p[0]*m_kernel0    (c,modif);
			double y = m_p[1]*m_kernel1.pdf(c,modif);
			return y/x;
		} else {
			m_kernel_id = 1;
			double x = m_p[1]*m_kernel1    (c,modif);
			double y = m_p[0]*m_kernel0.pdf(c,modif);
			return y/x;
		}
	}
};


class uniform_death_kernel {
	typedef boost::variate_generator<RJMCMCRandom&, boost::uniform_smallint<> > die_type;
public:
	uniform_death_kernel() {}
	template<typename Configuration, typename Modification>
	double operator()(Configuration& c, Modification& modif) const
	{
		modif.clear();
		if(c.empty()) return 1.;
		typename Configuration::iterator it = c.begin();
		die_type die(GetRandom(), boost::uniform_smallint<>(0,c.size()-1));
		std::advance(it, die());
		modif.insert_death(it);
		return 1./c.size();
	}
	template<typename Configuration, typename Modification>
	double pdf(const Configuration& c, const Modification& modif) const
	{
		if(modif.birth_size()!=1 || modif.death_size()!=0) return 0.;
		return 1./(c.size()+1);
	}
};

template<typename Generator>
class uniform_birth_kernel {
	typedef boost::variate_generator<RJMCMCRandom&, boost::uniform_real<> > die_type;
	Generator m_generator;
	mutable die_type m_die;
public:
	uniform_birth_kernel(const Generator& generator) : 
		m_generator(generator),
		m_die(GetRandom(), boost::uniform_real<>(0,1)) {}

	template<typename Configuration, typename Modification>
	double operator()(Configuration& c, Modification& modif) const
	{
		typedef typename Configuration::value_type T;
		T res;
		uniform_random_type_init(res);
		double p = rjmcmc::apply_visitor(m_generator,res);
		modif.insert_birth(res);
		return p/variant_size<T>::value;
	}

	template<typename Configuration, typename Modification>
	double pdf(const Configuration& c, const Modification& modif) const
	{
		typedef typename Configuration::value_type T;
		if(modif.birth_size()!=0 || modif.death_size()!=1) return 0.;
		double p = rjmcmc::apply_visitor(m_generator.pdf(),c[*modif.death_begin()]);
		return p/variant_size<T>::value;
	}
};

class unary_kernel {
	double m_p;
public:
	enum { size = 1 };
	unary_kernel(double p=1.) : m_p(p) {}
	inline unsigned int kernel_id() const { return 0; }
	inline void probability(double p) { m_p = p; }
	inline double probability() const { return m_p; }
	inline void probability(unsigned int, double p) { m_p = p; }
	inline double probability(unsigned int) const { return m_p; }
};


template<typename Modifier>
class modification_kernel : public unary_kernel {
	Modifier m_modifier;
	typedef boost::variate_generator<RJMCMCRandom&, boost::uniform_smallint<> > die_type;
public:
	modification_kernel(const Modifier& m, double p=1) : unary_kernel(p), m_modifier(m) {}

	template<typename Configuration, typename Modification>
	double operator()(Configuration& c, Modification& modif) const
	{
		modif.clear();
		if(c.empty()) return 1.;
		typedef typename Configuration::value_type T;
		typename Configuration::iterator it = c.begin();
		die_type cdie(GetRandom(), boost::uniform_smallint<>(0,c.size()-1));
		std::advance(it, cdie());
		modif.insert_death(it);

		//todo: sortir ces lignes specifiques de la lib
		typedef std::pair<Rectangle_2,Rectangle_2> Rectangle_2_pair;
		boost::variant<Rectangle_2,Circle_2,Rectangle_2_pair
		> res;

		uniform_random_type_init(res);
		double green_ratio = rjmcmc::apply_visitor(m_modifier,c[it],res);


		//todo: sortir ces lignes specifiques de la lib
		Rectangle_2      *r=boost::get<Rectangle_2>(&res);
		Circle_2         *n=boost::get<Circle_2>(&res);
		Rectangle_2_pair *p=boost::get<Rectangle_2_pair>(&res);
		if(r) {
			modif.insert_birth(*r);
		}
		if(n) {
			modif.insert_birth(*n);
		}
		if(p) {
			modif.insert_birth(p->first);
			modif.insert_birth(p->second);
		}

		return green_ratio;
	}
};

// variadic template support would hide the std::tuple implementation detail
#if 0
	#define IF_VARIADIC(x,y) x
#else
	#define IF_VARIADIC(x,y) y
#endif

template<typename IF_VARIADIC(K...,Kernels_)>
class sampler
{
	IF_VARIADIC(typedef std::tuple<K...> Kernels_, )
public:
	typedef Kernels_ Kernels;

// constructor
	sampler(const IF_VARIADIC(K&...,Kernels&) k) :
		m_kernel(IF_VARIADIC(k...,k)), m_die(GetRandom(), boost::uniform_real<>(0,1))
	{}

// kernel accessors
	enum { size = boost::tuples::length<Kernels>::value };

	template<unsigned int N>
	inline const typename boost::tuples::element<N, Kernels>::type& kernel() const {
		return boost::tuples::get<N>(m_kernel);
	}
	template<unsigned int N>
	inline typename boost::tuples::element<N, Kernels>::type& kernel() {
		return boost::tuples::get<N>(m_kernel);
	}

// statistics accessors
	inline double delta() const { return m_delta; }
	inline double green_ratio() const { return m_green_ratio; }
	inline int kernel_id() const { return m_kernel_id; }
	inline bool accepted() const { return m_accepted; }

// main sampling function
	template<typename Configuration> void operator()(Configuration &c, double temp)
	{
		typename Configuration::modification modif;
		m_kernel_id   = internal::random_apply(m_die(),m_kernel,c,modif,m_green_ratio);
		m_delta       = c.delta_energy(modif);
		m_green_ratio*= exp(-m_delta/temp);
		m_accepted    = ( m_die() < m_green_ratio );
		if (m_accepted) c.apply(modif);
	}

private:
	Kernels m_kernel;
	boost::variate_generator<RJMCMCRandom&, boost::uniform_real<> > m_die;
	int     m_kernel_id;
	double  m_delta;
	double  m_green_ratio;
	bool    m_accepted;
};

}; // namespace rjmcmc

#endif // __RJMCMC_SAMPLER_HPP__
