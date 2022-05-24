.. _ncs_2.0.0_migration:

Migration notes for |NCS| v2.0.0
################################

.. contents::
   :local:
   :depth: 2

In version 2.0.0, the |NCS| introduced a number of breaking changes into its codebase.
If your application was built using |NCS| v1.x.x, make sure to complete the actions listed in the following sections to migrate your application to |NCS| v2.x.x.

Toolchain change
****************

|NCS| no longer uses the GNU Arm Embedded Toolchain.
The current default toolchain is the Zephyr toolchain.

*Required action*: Install the new toolchain by following the steps in "Install a Toolchain" in Zephyr's :ref:`zephyr:getting_started`.

|VSC| extension IDE is now the default
**************************************

|VSC| extension IDE has replaced SEGGER Embedded Studio as the default supported IDE for working with the |NCS|.

*Required action*: If you are building the application or sample using SEGGER Embedded Studio or on the command line and want to migrate to |VSC| extension IDE, follow the instructions in the `migrating from other IDEs to VS Code <Migrating IDE_>`_ documentation.

Pin control transition
**********************

*Required action*:

TF-M replaces SPM as the default secure firmware solution
*********************************************************

Trusted Firmware-M (TF-M) is now used in |NCS| as a default solution for running an application from the non-secure area of the memory.

*Required action*:

Switch from TinyCBOR to zcbor
*****************************

The TinyCBOR to zcbor migration guide is provided here :ref:`migration_tinycbor_to_zcbor`
*Required action*:

New logging mechanism
*********************

*Required action*:

Zephyr namespace change
***********************

*Required action*:
