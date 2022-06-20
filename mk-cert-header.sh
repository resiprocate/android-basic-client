#!/bin/bash
set -e

CERT_SRC=/etc/ssl/certs/ca-certificates.crt

CERTS_TMP=`mktemp`
sed -e 's/^\(.*\)$/"&\\n"/' \
  ${CERT_SRC} > ${CERTS_TMP}

CERTS_HXX=jni/RootCertificates.hxx


cat << EOF > ${CERTS_HXX}
#ifndef __RootCertificates_hxx
#define __RootCertificates_hxx

// source: ${CERT_SRC}

// generated: `date`

char *rootCertificates = `cat ${CERTS_TMP}`;

#endif
EOF

rm ${CERTS_TMP}

