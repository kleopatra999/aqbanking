/* This file is auto-generated from "transactionlimits.xml" by the typemaker
   tool of Gwenhywfar. 
   Do not edit this file -- all changes will be lost! */
#ifndef TRANSACTIONLIMITS_L_H
#define TRANSACTIONLIMITS_L_H

/** @page P_AB_TRANSACTION_LIMITS_LIB AB_TransactionLimits (lib)
This page describes the properties of AB_TRANSACTION_LIMITS
This type describes the limits for fields of an @ref AB_TRANSACTION. The limits have the following meanings:

<ul>
<li>
maxLenSOMETHING: if 0 then this limit is unknown, if -1 then the described element is not allowed to be set in the transaction. All other values represent the maximum lenght of the described field.

</li>

<li>
minLenSOMETHING: if 0 then this limit is unknown. All other values represent the minimum lenght of the described field.

</li>

<li>
maxLinesSOMETHING: if 0 then this limit is unknown All other values represent the maximum number of lines for the described field.

</li>

<li>
minLinesSOMETHING: if 0 then this limit is unknown. All other values represent the minimum number of lines for the described field.

</li>

<li>
valuesSOMETHING: A list of allowed values (as string). If this list is empty then there all values are allowed (those lists @b exist in any case, so the appropriate getter function will never return NULL).

</li>

</ul>

So if you want to check whether an given field is at all allowed you must check whether &quot;maxLenSOMETHING&quot; has a value of &quot;-1&quot;.
<h3>Issuer Name</h3>
<p>
Limits for the issuer name.
</p>
<h3>Payee Name</h3>
<p>
Limits for the payee name.
</p>
<h3>Local Bank Code</h3>
<p>
Limits for local bank code.
</p>
<h3>Local Account Id</h3>
<p>
Limits for local account id.
</p>
<h3>Local Account Number</h3>
<p>
Limits for local account id suffix.
</p>
<h3>Remote Bank Code</h3>
<p>
Limits for remote bank code.
</p>
<h3>Remote Account Number</h3>
<p>
Limits for remote account number.
</p>
<h3>Remote Account Number Suffix</h3>
<p>
Limits for remote account id suffix.
</p>
<h3>Remote IBAN</h3>
<p>
Limits for remote IAN.
</p>
<h3>Text Key</h3>
<p>
Limits for textKey.
</p>
<h3>Customer Reference</h3>
<p>
Limits for customer reference.
</p>
<h3>Bank Reference</h3>
<p>
Limits for bank reference.
</p>
<h3>Purpose</h3>
<p>
Limits for purpose (called

<i>
memo

</i>

in some apps).
</p>
*/
#include "transactionlimits.h"

#ifdef __cplusplus
extern "C" {
#endif



/** @name Issuer Name
 *
Limits for the issuer name.
*/
/*@{*/



/*@}*/

/** @name Payee Name
 *
Limits for the payee name.
*/
/*@{*/





/*@}*/

/** @name Local Bank Code
 *
Limits for local bank code.
*/
/*@{*/



/*@}*/

/** @name Local Account Id
 *
Limits for local account id.
*/
/*@{*/



/*@}*/

/** @name Local Account Number
 *
Limits for local account id suffix.
*/
/*@{*/



/*@}*/

/** @name Remote Bank Code
 *
Limits for remote bank code.
*/
/*@{*/



/*@}*/

/** @name Remote Account Number
 *
Limits for remote account number.
*/
/*@{*/



/*@}*/

/** @name Remote Account Number Suffix
 *
Limits for remote account id suffix.
*/
/*@{*/



/*@}*/

/** @name Remote IBAN
 *
Limits for remote IAN.
*/
/*@{*/



/*@}*/

/** @name Text Key
 *
Limits for textKey.
*/
/*@{*/




/*@}*/

/** @name Customer Reference
 *
Limits for customer reference.
*/
/*@{*/



/*@}*/

/** @name Bank Reference
 *
Limits for bank reference.
*/
/*@{*/



/*@}*/

/** @name Purpose
 *
Limits for purpose (called

<i>
memo

</i>

in some apps).
*/
/*@{*/





/*@}*/


#ifdef __cplusplus
} /* __cplusplus */
#endif


#endif /* TRANSACTIONLIMITS_L_H */