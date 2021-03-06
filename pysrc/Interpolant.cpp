/* -*- c++ -*-
 * Copyright (c) 2012-2014 by the GalSim developers team on GitHub
 * https://github.com/GalSim-developers
 *
 * This file is part of GalSim: The modular galaxy image simulation toolkit.
 * https://github.com/GalSim-developers/GalSim
 *
 * GalSim is free software: redistribution and use in source and binary forms,
 * with or without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions, and the disclaimer given in the accompanying LICENSE
 *    file.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the disclaimer given in the documentation
 *    and/or other materials provided with the distribution.
 */
#ifndef __INTEL_COMPILER
#if defined(__GNUC__) && __GNUC__ >= 4 && (__GNUC__ >= 5 || __GNUC_MINOR__ >= 8)
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif
#endif

#define BOOST_NO_CXX11_SMART_PTR
#include "boost/python.hpp"
#include <cstdlib>
#include "Interpolant.h"

namespace bp = boost::python;

namespace galsim {

    struct PyInterpolant
    {

        static Interpolant* ConstructInterpolant(std::string str)
        {
            // Make it lowercase
            std::transform(str.begin(), str.end(), str.begin(), ::tolower);

            // Return the right Interpolant according to the given string.
            if (str == "delta") return new Delta();
            else if (str == "nearest") return new Nearest();
            else if (str == "sinc") return new SincInterpolant();
            else if (str == "linear") return new Linear();
            else if (str == "cubic") return new Cubic();
            else if (str == "quintic") return new Quintic();
            else if (str.substr(0,7) == "lanczos") {
                int n = strtol(str.substr(7).c_str(),0,0);
                if (n <= 0) {
                    PyErr_SetString(PyExc_TypeError, "Invalid Lanczos order");
                    bp::throw_error_already_set();
                }
                return new Lanczos(n);
            } else {
                PyErr_SetString(PyExc_TypeError, "Invalid interpolant string");
                bp::throw_error_already_set();
                return 0;
            }
        }

        static Interpolant2d* ConstructInterpolant2d(const std::string& str)
        {
            boost::shared_ptr<Interpolant> i1d(ConstructInterpolant(str));
            return new InterpolantXY(i1d);
        }

        static void wrap()
        {
            // We wrap Interpolant classes as opaque, construct-only objects; we just
            // need to be able to make them from Python and pass them to C++.
            // The only exception is that we make the 'uval' method accessible in python, since the
            // Fourier transform of the interpolant can be useful for some purposes (e.g.,
            // modifications of power spectra of objects that have been interpolated).
            bp::class_<Interpolant,boost::noncopyable>("Interpolant", bp::no_init)
                .def("__init__", bp::make_constructor(
                        &ConstructInterpolant, bp::default_call_policies(), bp::arg("str")))
                .def("uval", &Interpolant::uval, (bp::arg("uval")=0));
            bp::class_<Interpolant2d,boost::noncopyable>("Interpolant2d", bp::no_init)
                .def("__init__", bp::make_constructor(
                        &ConstructInterpolant2d, bp::default_call_policies(), bp::arg("str")))
                .def("uval", &Interpolant2d::uval, (bp::arg("u")=0,bp::arg("v")=0));
            bp::class_<InterpolantXY,bp::bases<Interpolant2d>,boost::noncopyable>(
                "InterpolantXY",
                bp::init<boost::shared_ptr<Interpolant> >(bp::arg("i1d"))
            );

            static const char* delta_doc =
            "Delta-function interpolant in 1d: The interpolant for when you do not want to\n"
            "interpolate between samples.  It is not really intended to be used for any analytic\n"
            "drawing because it is infinite in the x domain at the location of samples, and it\n"
            "extends to infinity in the u domain.  But it could be useful for photon-shooting,\n"
            "where it is trivially implemented as no displacements.\n\n"
            "The `width` argument in the constructor is used to make a crude box approximation\n"
            "to the x-space delta function and to give a large but finite urange\n"
            "(default `width=1e-3`).\n";
            bp::class_<Delta,bp::bases<Interpolant>,boost::noncopyable>(
                "Delta", delta_doc, bp::init<double>(bp::arg("width")=1E-3)
            );

            static const char* nearest_doc =
            "Nearest-neighbor interpolation (boxcar): The nearest-neighbor interpolant performs\n"
            "poorly as a k-space or x-space interpolant for interpolated images.  (See paper by\n"
            "Bernstein & Gruen, http://arxiv.org/abs/1401.2636.)  The\n"
            "objection to its use in Fourier space does not apply when shooting photons to\n"
            "generate an image; in that case, the nearest-neighbor interpolant is quite efficient\n"
            "(but not necessarily the best choice in terms of accuracy).\n\n"
            "Tolerance `tol` determines how far onto sinc wiggles the uval will go.  Very far, by\n"
            "default! (default `tol=1e-3`)\n";
            bp::class_<Nearest,bp::bases<Interpolant>,boost::noncopyable>(
                "Nearest", nearest_doc, bp::init<double>(bp::arg("tol")=1E-3)
            );

            static const char* sinc_doc =
            "Sinc interpolation (inverse of nearest-neighbor): The Sinc interpolant\n"
            "(K(x) = sin(pi x)/(pi x)) is mathematically perfect for band-limited data,\n"
            "introducing no spurious frequency content beyond kmax = pi/dx for input data with\n"
            "pixel scale dx.  However, it is formally infinite in extent and, even with reasonable\n"
            "trunction, is still quite large.  It will give exact results in\n"
            "SBInterpolatedImage::kValue() when it is used as a k-space interpolant, but is\n"
            "extremely slow.  The usual compromise between sinc accuracy vs. speed is the Lanczos\n"
            "interpolant (see its documentation for details).\n\n"
            "Tolerance `tol` determines how far onto sinc wiggles the xval will go.  Very far, by\n"
            "default! (default `tol=1e-3`)\n";
            bp::class_<SincInterpolant,bp::bases<Interpolant>,boost::noncopyable>(
                "SincInterpolant", sinc_doc, bp::init<double>(bp::arg("tol")=1E-3)
            );

            static const char* linear_doc =
            "Linear interpolant: The linear interpolant is a poor choice for FFT-based operations\n"
            "on interpolated images, as it rings to high frequencies.  (See Bernstein & Gruen,\n"
            "http://arxiv.org/abs/1401.2636.)  This objection does not apply\n"
            "when shooting photons, in which case the linear interpolant is quite efficient (but\n"
            "not necessarily the best choice in terms of accuracy).\n\n"
            "Tolerance `tol` determines how far onto sinc^2 wiggles the uval will go.  Very far,\n"
            "by default! (default `tol=1e-3`)\n";
            bp::class_<Linear,bp::bases<Interpolant>,boost::noncopyable>(
                "Linear", linear_doc, bp::init<double>(bp::arg("tol")=1E-3)
            );

            static const char* lanczos_doc =
            "The Lanczos interpolation filter, nominally sinc(x)*sinc(x/n): The Lanczos filter\n"
            "is an approximation to the band-limiting sinc filter with a smooth cutoff at high x.\n"
            "Order n Lanczos has a range of +/- n pixels.  It typically is a good compromise\n"
            "between kernel size and accuracy.\n\n"
            "The filter has accuracy parameters `xvalue_accuracy` and `kvalue_accuracy` that\n"
            "relate to the accuracy of building the initial lookup table.  For now, these are\n"
            "fixed in src/Interpolant.cpp to be 0.1 times the input `tol` value, where `tol` is\n"
            "typically very small already (default `tol=1e-4`).\n\n"
            "Note that pure Lanczos, when interpolating a set of constant-valued samples, does\n"
            "not return this constant.  Setting `conserve_dc` in the constructor tweaks the\n"
            "function so that it approximately conserves the value of constant (DC) input data\n"
            "(accurate to better than 1.e-5 when used in two dimensions).\n";
            bp::class_<Lanczos,bp::bases<Interpolant>,boost::noncopyable>(
                "Lanczos", lanczos_doc, bp::init<int,bool,double>(
                    (bp::arg("n"), bp::arg("conserve_dc")=true, bp::arg("tol")=1E-4)
                )
            );

            static const char* cubic_doc =
            "Cubic interpolator exact to 3rd order Taylor expansion (from R. G. Keys,\n"
            "IEEE Trans. Acoustics, Speech, & Signal Proc 29, p 1153, 1981\n\n"
            "The cubic interpolant is a reasonable choice for a four-point interpolant for\n"
            "interpolated images.  (See Bernstein & Gruen, http://arxiv.org/abs/1401.2636.)\n\n"
            "Default tolerance parameter `tol=1e-4`.\n";
            bp::class_<Cubic,bp::bases<Interpolant>,boost::noncopyable>(
                "Cubic", cubic_doc, bp::init<double>(bp::arg("tol")=1E-4)
            );

            static const char* quintic_doc =
            "Piecewise-quintic polynomial interpolant, ideal for k-space interpolation:\n"
            "See Bernstein & Gruen, http://arxiv.org/abs/1401.2636.\n\n"
            "Default tolerance parameter `tol=1e-4`.\n";
            bp::class_<Quintic,bp::bases<Interpolant>,boost::noncopyable>(
                "Quintic", quintic_doc, bp::init<double>(bp::arg("tol")=1E-4)
            );
        }

    };
 
    void pyExportInterpolant()
    { PyInterpolant::wrap(); }

} // namespace galsim
