/* Paranormal - A highly customizable audio visualization library
 * Copyright (C) 2001  Jamie Gennis <jgennis@mindspring.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>

#include "pnactuatorfactory.h"

#include "pnactuatorlist.h"
#include "pnblur.h"
#include "pndisplacement.h"
#include "pndistortion.h"
#include "pnflip.h"
#include "pnimagecontext.h"
#include "pnrotozoom.h"
#include "pnscope.h"

/* This NEEDS to be kept in sync with ALL the actuators */
void
pn_builtins_register (void)
{
  pn_actuator_factory_register_actuator ("Container.Actuator_List", PN_TYPE_ACTUATOR_LIST);
  pn_actuator_factory_register_actuator ("Transform.Blur", PN_TYPE_BLUR);
  pn_actuator_factory_register_actuator ("Transform.Displacement", PN_TYPE_DISPLACEMENT);
  pn_actuator_factory_register_actuator ("Transform.Distortion", PN_TYPE_DISTORTION);
  pn_actuator_factory_register_actuator ("Transform.Flip", PN_TYPE_FLIP);
  pn_actuator_factory_register_actuator ("Container.Image_Context", PN_TYPE_IMAGE_CONTEXT);
  pn_actuator_factory_register_actuator ("Transform.Roto_Zoom", PN_TYPE_ROTO_ZOOM);
  pn_actuator_factory_register_actuator ("Render.Scope", PN_TYPE_SCOPE);
}
