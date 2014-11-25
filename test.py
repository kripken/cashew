#!/usr/bin/env python2
# -*- Mode: python -*-

import os, sys
from subprocess import Popen, PIPE, STDOUT

for i in os.listdir('samples'):
  if i.endswith('.js'):
    expected = open(os.path.join('samples', i.replace('.js', '.txt'))).read()
    out, err = Popen(['./a.out', os.path.join('samples', i)], stdout=PIPE).communicate()
    out = out.split('output:\n')[1]
    out = out.replace('\n\n', '\n')
    expected = expected.replace('\n\n', '\n')
    #print out
    #print '^v'
    #print expected
    assert out == expected, [out, expected]

print 'ok.'
