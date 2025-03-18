#!/usr/bin/env python
import re, os, glob
try:
    from setuptools import setup, Extension
except ImportError:
    from ez_setup import use_setuptools
    use_setuptools()
    from setuptools import setup, Extension


f = open(os.path.join(os.path.dirname(__file__), "README.md"))
yref_readme = f.read()
f.close()

# Define the C extension module
yref = Extension(
    'yref',  # The name of the module
    sources=[
        'yref/yref.c',
        'yref/refer.c',
        'yref/entries.c',
        'yref/yaml2json.c',
        'yref/cJSON.c',
        'yref/find_links.c',
        'yref/json2css.c',
        'yref/json2yaml.c',
    ],
    libraries=['yaml', 'yajl'],
)

setup(
    name="yref",
    version='1.0.1',
    description="Python package to handle xml references",
    long_description=yref_readme,
    long_description_content_type='text/markdown',
    packages=["yref"],
    author='Vasyl Yovdiy',
    author_email='yovdiyvasyl@gmail.com',
    url="https://github.com/y-vas/yref",
    python_requires='>=3.7',  # Your supported Python ranges
    setup_requires=["setuptools"],
    ext_modules=[yref],  # Add the C extension module
    keywords=[
        "yref",
    ],
    license="GNU",
    classifiers=[
        "Topic :: Utilities",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: Implementation :: PyPy",
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: End Users/Desktop",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: MIT License",
        "Topic :: Scientific/Engineering",
        "Topic :: Software Development",
        "Topic :: Software Development :: Libraries",
        "Topic :: Scientific/Engineering",
        "Topic :: Utilities",
    ],
)
