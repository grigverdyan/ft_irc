#ifndef NSTD_NULLPTR_T_H
#define NSTD_NULLPTR_T_H

// namespace nostandard
namespace nstd {

const class nullptr_t 
{
public:
    // Allow implicit conversion to any type of null non-member pointer
    template<class T>
    operator T*() const
    {
        return 0;
    }

    // Allow implicit conversion to any type of null member pointer
    template<class C, class T>
    operator T C::*() const {
        return 0;
    }

private:
    // Disallow taking the address of this object
    void operator&() const;

};

} // namespace nstd

const nstd::nullptr_t nullptr = {};

#endif // NSTD_NULLPTR_T_H