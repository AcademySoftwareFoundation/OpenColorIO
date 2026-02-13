// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_PROPHOTO_RGB_H
#define INCLUDED_OCIO_PROPHOTO_RGB_H


namespace OCIO_NAMESPACE
{

	class BuiltinTransformRegistryImpl;

	namespace PROPHOTO
	{

		void RegisterAll(BuiltinTransformRegistryImpl& registry) noexcept;

	} // namespace PROPHOTO

} // namespace OCIO_NAMESPACE


#endif // INCLUDED_OCIO_PROPHOTO_RGB_H
