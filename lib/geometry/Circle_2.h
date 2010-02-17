#ifndef __Circle_2_H__
#define __Circle_2_H__

#if USE_CGAL

#include <CGAL/Circle_2.h>

CGAL_BEGIN_NAMESPACE
template<class K>
inline typename K::FT radius(const CGAL::Circle_2<K>& c) {
	return CGAL::sqrt(c.squared_radius());
}
CGAL_END_NAMESPACE

#else

CGAL_BEGIN_NAMESPACE

template<class K_> class Circle_2
{
public:
	typedef K_ K;
	typedef typename K::RT RT;
	typedef typename K::FT FT;
	typedef typename K::Point_2 Point_2;
	typedef typename K::Vector_2 Vector_2;
	typedef typename K::Line_2 Line_2;
	typedef Circle_2 Self;

	Circle_2()
	{}
	Circle_2(const Point_2 &center, double radius) : m_center(center), m_radius(radius), m_squared_radius(radius*radius)
	{}

	inline Point_2 center() const
	{	return m_center;}
	inline bool is_degenerate() const
	{	return m_radius==0;}
	inline FT radius() const
	{	return m_radius;}
	inline FT squared_radius() const
	{	return m_squared_radius;}

private :
	Point_2 m_center;
	FT m_radius, m_squared_radius;
};


template < class K >
std::ostream &
operator<<(std::ostream &os, const Circle_2<K> &c)
{
	switch(os.iword(IO::mode))
	{
		case IO::ASCII :
		return os << c.center() << ' ' << c.radius();
		case IO::BINARY :
		return os << c.center() << c.radius();
		default:
		return os << "Circle_2(" << c.center() << ", " << c.radius() << ")";
	}
}

template < class K >
std::istream &
operator>>(std::istream &is, Circle_2<K> &b)
{
	typename K::Point_2 p;
	typename K::FT r;

	is >> p >> r;

	if (is)
	b = Circle_2<K>(p, r);
	return is;
}

template<class K>
typename K::FT radius(const CGAL::Circle_2<K>& c) {
	return c.radius();
}

CGAL_END_NAMESPACE

#endif // USE_CGAL


CGAL_BEGIN_NAMESPACE

template<class K>
inline typename K::FT perimeter(const CGAL::Circle_2<K>& c) {
	return 2*M_PI*radius(c);
}


template<class K>
inline typename K::FT area(const CGAL::Circle_2<K>& c) {
	return M_PI*c.squared_radius();
}
CGAL_END_NAMESPACE

#endif //#ifndef __Circle_2_H__
