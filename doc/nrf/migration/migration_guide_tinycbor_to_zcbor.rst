.. _migration_tinycbor_to_zcbor

TinyCBOR to zcbor migration guide
#################################

.. _migration_tinycbor_to_zcbor_general

General migration guide
***********************

The TinyCBOR to zcbor migration task consists of two tasks:
    1. :ref:`migration_tinycbor_to_zcbor_general_init`
    2. :ref:`migration_tinycbor_to_zcbor_general_api`

The first task a little bit more complicated, the second task it basically function to function
conversion, with a few exceptions how parameters are passed and what return values are returned
from API calls.

.. _migration_tinycbor_to_zcbor_general_init

zcbor state initialization
==========================

Decoding state initialization
-----------------------------

Decoding with TinyCBOR requires setting up a reader with a buffer by calling 
`cbor_buf_reader_init`, and attaching it to a parser state with `cbor_parser_init`.
zcbor decoding is initialized with call to `zcbor_new_decode_state` that directly ties
provided buffer to decoding state.
For better understanding of how zcbor states work please refer to the zcbor documentation.
Below is an example of simple zcbor state initialization, for decoding:

.. code-block::c

    #define N   20              /* Max expected encoded elements */
    zcbor_state_t zs[2];
    uint8_t buffer[SOME_SIZE];

    /* Read some data to the buffer */
    ...

    zcbor_new_decode_state(zs, ARRAY_SIZE(zs), buffer, ARRAY_SIZE(buffer), N);
    /* Decode data from the buffer with zs state */
    ...

In the above example the zcbor state `zs` consists of two states, this is minimal reasonable
state count, that will allow to work on flat CBOR encoding where no variables are embedded within
lists or maps. Two states are used because one will be current state the other may be used
as constant state where encoding error is stored, and so on.
In case when you plan to have embedded lists and/or maps the number of states, for decoding,
needs to be increased by adding a number of levels to 2.
The `buffer` may be any plain buffer and decoding requires buffer size to know where an
encoded data ends. The `N` represents possible number of encoded fields, it does not have to
be accurate it needs to be at least number of expected fields; in case when top element is
a list or a map, then this can be 1, mcumgr is an example of such case where there is always
top level map required.
At this point the zcbor state is initialized and following it zcbor decoding API calls should be
given a pointer to the first zcbor state, so basically `zs`, or `&zs[0]` if you prefer, from
the example.

.. note::

    Currently there is no API provided, for zcbor, to get an info on how much of an input buffer
    has been consumed, for decoding, and the only way to find out is to substitute the buffer
    pointer, given at a zcbor state initialization, from the state object element `payload_mut`,
    for example `zs->payload_mut - buffer`.

Encoding state initialization
-----------------------------

Encoding with TinyCBOR is similar to decoding and requires two API calls, one to setup a buffer
writer, `cbor_buf_writer_init`, and one to attach writer to encoding state `cbor_encoder_init`.
zcbor encoding is initialized with single call to `zcbor_new_encode_state` that directly ties
provided buffer to encoding state.
For better understanding of how zcbor states work please refer to the zcbor documentation.
Below is an example of simple zcbor state initialization, for encoding:

.. code-block::c

    zcbor_state_t zs[2];
    uint8_t buffer[SOME_SIZE];

    zcbor_new_encode_state(zs, ARRAY_SIZE(zs), buffer, ARRAY_SIZE(buffer), 0);
    /* Encode data to the buffer with zs state */
    ...

In the above example zcbor state `zs` consists of two states, which is enough when zcbor is in
non-canonical configuration (see :kconfig:option:`ZCBOR_CANONICAL`), otherwise number of stats
would need to be increased by maximum level of expected lists and arrays.
The other parameters given are the `buffer` where data will be placed, with size that limits it,
and 0 as final parameter - which represents number of items encoded into the buffer.

At this point the zcbor state is initialized and following it zcbor decoding API calls should be
given a pointer to the first zcbor state, so basically `zs`, or `&zs[0]` if you prefer, from
the example.

.. note::

    Currently there is no API provided, for zcbor, to get an info on how much of an output buffer
    has been used, by encoded data, and the only way to find out is to substitute the buffer
    pointer, given at a zcbor state initialization, from the state object element `payload_mut`,
    for example `zs->payload_mut - buffer`.

.. _migration_tinycbor_to_zcbor_general_api

TinyCBOR to zcbor decoding/encoding API replacement
---------------------------------------------------

The replacement is quite straightforward with three major differences between APIs being:
   
    1. TinyCBOR calls return error codes while zcbor returns `bool` value (`true` is success);
    2. zcbor does not provide type/value peek functions;
    3. all of zcbor functions advance buffer on success.

zbcor stores error codes with `zs` and, when configured, may automatically fail consecutive
calls when decoding/encoding fails.
Otherwise calls are quite similar: functions from both APIs takes context/state as one parameter
and value or pointer for value and optional buffer size as following parameters.

Below is, incomplete, replacement list that maps TinyCBOR functions to zcbor equivalent:

.. table::
    :align:center

    +===================================+===================================+
    | TinyCBOR                          | zcbor                             |
    +-----------------------------------+-----------------------------------+
    | `cbor_value_enter_container`      | `zcbor_map_start_decode`          |
    |                                   | `zcbor_list_start_decode`         |
    +-----------------------------------+-----------------------------------+
    | `cbor_value_leave_container`      | `zcbor_map_end_decode`            |
    |                                   | `zcbor_list_end_decode`           |
    +-----------------------------------+-----------------------------------+
    | `cbor_value_get_int`              | `zcbor_int32_decode`              |
    +-----------------------------------+-----------------------------------+
    | `cbor_value_get_int64`            | `zcbor_int64_decode`              |
    +-----------------------------------+-----------------------------------+
    | `cbor_value_get_uint64'           | `zcbor_uint6_decode`              |
    +-----------------------------------+-----------------------------------+
    | `cbor_value_get_boolean`          | `zcbor_bool_decode`               |
    +-----------------------------------+-----------------------------------+
    | `cbor_value_get_float`            | `zcbor_float32_decode`            |
    +-----------------------------------+-----------------------------------+
    | `cbor_value_get_double`           | `zcbor_float64_decode`            |
    |                                   | `zcbor_double_decode`             |
    +-----------------------------------+-----------------------------------+
    | `cbor_value_get_string_length`    | No direct replacement, the        |
    |                                   | `zcbor_bstr_decode` or            |
    |                                   | `zcbor_tstr_decode` will retrieve |
    |                                   | both the value pointer and length |
    |                                   | in a single call.                 |
    +-----------------------------------+-----------------------------------+
    | `cbor_value_copy_text_string`     | No direct replacement, the        |
    |                                   | `zcbor_tstr_decode` will retrieve |
    |                                   | both the value pointer and length |
    |                                   | in a single call.                 |
    +-----------------------------------+-----------------------------------+
    | `zcbor_value_copy_byte_string`    | No direct replacement, the        |
    |                                   | `zcbor_bstr_decode` will retrieve |
    |                                   | both the value pointer and length |
    |                                   | in a single call.                 |
    +-----------------------------------+-----------------------------------+
    | `cbor_encode_create_array`        | `zcbor_list_start_encode`         |
    +-----------------------------------+-----------------------------------+
    | `cbor_encode_create_map`          | `zcbor_map_start_encode`          |
    +-----------------------------------+-----------------------------------+
    | `cbor_encoder_close_container`    | `zcbor_map_end_encode`            |
    |                                   | `zcbor_list_end_encode`           |
    +-----------------------------------+-----------------------------------+
    | `cbor_encode_int`                 | `zcbor_int32_put`                 |
    |                                   | `zcbor_int64_put`                 |
    +-----------------------------------+-----------------------------------+
    | `cbor_encode_uint`                | `zcbor_uint32_put`                |
    |                                   | `zcbor_uint64_put`                |
    +-----------------------------------+-----------------------------------+
    | `cbor_enocode_boolean`            | `zcbor_bool_put`                  |
    +-----------------------------------+-----------------------------------+
    | `cbor_encode_half_float`          | no equivalent                     |
    +-----------------------------------+-----------------------------------+
    | `cbor_encode_float`               | `zcbor_float32_put`               |
    +-----------------------------------+-----------------------------------+
    | `cbor_encode_double               | `zcbor_float64_put`               |
    |                                   | `zcbor_double_put`                |
    +-----------------------------------+-----------------------------------+
    | `cbor_encode_text_string`         | `zcbor_tstr_encode`               |
    |                                   | `zcbor_tstr_encode_ptr`           |
    |                                   | `zcbor_tstr_put_lit`              |
    +-----------------------------------+-----------------------------------+
    | `cbor_encode_text_stringz`        | `zcbor_tstr_put_term`             |
    +-----------------------------------+-----------------------------------+
    | `cbor_encode_byte_string`         | `zcbor_bstr_encode`               |
    |                                   | `zcbor_bstr_encode_ptr`           |
    +-----------------------------------+-----------------------------------+
    | `cbor_encode_null`                | `zcbor_nil_put`                   |
    +-----------------------------------+-----------------------------------+
    | `cbor_encode_undefined`           | `zcbor_undefined_put`             |
    +-----------------------------------+-----------------------------------+

Because there is not direct replacement for the `cbor_value_is_null` and `cbor_value_is_undefined`
to check whether such value is, where expected, user can utilize following code:

.. code:c:

    bool ok;
    ...

    /* Make sure that not in error state first */
    if (zs->constant_state->error != 0) {
        /* Already in error */
        ...
    }
    ok = zcbor_nil_decode(zs, NULL);
    if (!ok && zs->constant_state->error == ZCBOR_ERR_WRONG_TYPE) {
        /* Process non NULL element */
        zcbor_pop_error(zs);
        ...
    } else if (!ok) {
        /* Other error */
        ...
    }
    /* Successfully decoded NULL element in CBOR stream */
    ...

.. _migration_tinycbor_to_zcbor_mcumgr

MCUMGR commands and groups migration guide
******************************************

In contrast to :ref:`migration_tinycbor_to_zcbor_general`, except for tuning some of Kconfig
configuration constants, there is no need for zcbor setup as mcumgr library takes case of the
task.
Most of the task, which consists of function replacement, is actually covered by the
:ref:`migration_tinycbor_to_zcbor_general`, with exception to usage of `cborattr` which
is covered by paragraph :ref:`migration_tinycbor_to_zcbor_mcumgr_cborattr`.

The other important issue is decoding/encoding context access, which is described in
:ref:`migration_tinycbor_to_zcbor_mcumgr_ctxt`.

.. note::

    The encoding context given to a command processing handler function has the top level
    map already created, and will have the map closed on successful return from handler,
    so handlers need not create the top level map!
    In contrary to the above, the decoding context is not yet opened and handlers are
    responsible for opening the top level map before decoding of a command.

.. _migration_tinycbor_to_zcbor_mcumgr_ctxt

Decoding and encoding context
=============================

In TinyCBOR and zcbor implementation the mcumgr is providing, to command processing functions,
a pointer to an object of type `struct mgmt_ctxt` from which the `encoder` element would be used,
by CBOR encoding functions to write data and `it`,  TinyCBOR `CborValue iterator, to decoding
functions.
zcbor uses `zcbor_state_t` type objects for decoding and encoding states and these state objects
have replaced TinyCBOR `it` iterator and `encoder`, respectively, within `struct mgmt_ctxt`.
The `zcbor_state_t` is not directly placed in `struct mgmt_ctxt` but is embedded within
`struct cbor_nb_reader` type object, for decoding context, and in `struct cbor_nb_writer` type
object, for encoding context.
The `struct cbor_nb_reader` and `struct cbor_nb_writer` objects tie `zcbor_state_t` with
`net_buf` type buffers that hold data for decoding or will hold encoded data.

The replacement of decoder and encoder objects as accessed through `struct mgmt_ctxt` is
show by following table:

.. table::
    :align: center

    +===================+===================+
    | TinyCBOR          | zcbor             |   
    +-------------------+-------------------+
    | `encoder`         | `cnbe.ts`         |
    |                   | `cnbe->ts`        |
    +-------------------+-------------------+
    | `it`              | `cnde.ts`         |
    |                   | `cnde->ts`        |
    +---------------------------------------+

.. _migration_tinycbor_to_zcbor_mcumgr_cborattr

Replacing `cbor_read_object` with `zcbor_map_decode_bulk`
=========================================================

.. note::

There are some cases where decoding of a single key, or none, is required in which case this
paragraph is overkill, as a single key can be obtained by simple `zcbor_tst_decode`
looped until the key is found, but it is up to the user.

The `cborattr`, private mcumgr utility used within command processing code, to process
CBOR list contents in bulk has been replaced with `zcbor_bulk`, also private to mcumgr.
Both APIs are centered around main function `cbor_read_object`, in case of `TinyCBOR`,
and `zcbor_map_decode_bulk`, in case of zcbor.

The advantage of the `zcbor_map_decode_bulk`, over `cbor_read_object`, is simplification
of structures defining function decoding the CBOR.

To clarify how transition from `cbor_read_object` to `zcbor_map_decode_bulk`, here is an example

.. code-block::c

    int image;
    uint8_t img_data[SOME_DATA_LEN];
    size_t data_len;

    const struct cbor_attr_t off_attr[] = {
        [0] = {
            .attribute = "image",
            .type = CborAttrUnsignedIntegerType,
            .addr.uinteger = &req.image,
            .nodefault = true
        },
        [1] = {
            .attribute = "data",
            .type = CborAttrByteStringType,
            .addr.bytestring.data = img_data,
            .addr.bytestring.len = &data_len,
            .len = sizeof(img_data)
        },
        ...
    }

    rc = cbor_read_object(&ctxt->it, off_attr);
    ...

the `off_attr` is providing specification of data encoded in CBOR format that is decoded by the
call to `cbor_read_object`. The specification is a list of structures, that specify
expected list key (`.attribute` element), type of key (`.type`), pointer to buffer for it 
(usually subelement of `.addr` element) and few other attributes; the `cbor_read_object`
takes the specification and, using the TinyCBOR decoding context, it attempts to get all the
described fields into the specified designated variables.  After a successful call to
`cbor_read_object` the variables will be set and ready for further processing, and the buffer
will be shifted beyond the list; above included also copying binary and string buffers to specified
locations.
The context, accessed by the `ctxt->it` in above example, is provided by mcumgr library so it is
already initialized and have the buffer attached.

The zcbor conversion of the above code would look like this:

.. code-block::c

    int image;
    struct zcbor_string zst;    

    struct zcbor_map_decode_key_val image_upload_decode[] = {
            ZCBOR_MAP_DECODE_KEY_VAL(image, zcbor_int32_decode, &image),
            ZCBOR_MAP_DECODE_KEY_VAL(data, zcbor_bstr_decode, &zst),
    };

    ok = zcbor_map_decode_bulk(zsd, image_upload_decode,
                               ARRAY_SIZE(image_upload_decode), &decoded) == 0;

    ...


The list specification is significantly simplified as it consists of an array
`ZCBOR_MAP_DECODE_KEY_VAL` defined mappings each given name of key (without quotation marks),
a zcbor decoding function directly from API (or user provided type equivalent) and a pointer
to variable where extracted value will be stored.

.. note::

        The `zcbor_map_decode_bulk`, while going through the map, will start key matching with
        next key defined by `ZCBOR_MAP_DECODE_KEY_VAL` to the previous key find, which means
        that it is most efficient to define the mapping (the `zcbor_map_decode_key_val array),
        in order of predicted key:value appearence in a parsed message.

The difference here is that decoding of binary, and string data, does not involve copying of
the data to a specified buffer; instead, as described in :ref:`migration_tinycbor_to_zcbor_general`,
decoding function is paired with `struct zcbor_string` type variable, where decoding will store
position and length of the data, within provided CBOR buffer, and it is then up to the user
to copy the values to specified location. This is improvement in respect to RAM and CPU use
as a need for intermediate buffer has been, often, reduced.

The `zcbor_map_decode_bulk` takes the zcbor decoding context, `zsd` in above example, the mapping
specification and size of the mapping, and variable to return number of decoded fields.
Currently there is no method provided to figure out what fields did not get found in string,
so it is up to a user to verify, for example by checking if destination variable value has changed
from initial, whether keys that are optional have appeared in decoded string.
