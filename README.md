# Regex Matcher
[![Tests](https://github.com/sahabpardaz/regex-matcher/actions/workflows/maven.yml/badge.svg?branch=main)](https://github.com/sahabpardaz/regex-matcher/actions/workflows/maven.yml)
[![JitPack](https://jitpack.io/v/sahabpardaz/regex-matcher.svg)](https://jitpack.io/#sahabpardaz/regex-matcher)

This library provides facilities to match an input string against a collection of regex patterns.
This library acts as a wrapper around the popular `Chimera` library, which allows it to be used in Java.

Chimera is a software regular expression matching engine that is a hybrid of Hyperscan and PCRE. The design
goals of Chimera are to fully support PCRE syntax as well as to take advantage of the high performance
nature of Hyperscan. Hyperscan is a high-performance multiple regex matching library available as open source
with a C API.

You can learn more about them through the following links:
+ [Hyperscan Documentation](https://intel.github.io/hyperscan/dev-reference/intro.html)
+ [Chimera Documentation](https://intel.github.io/hyperscan/dev-reference/chimera.html)

## Build Process

You can get a released version of the Java jar file from this
[Jitpack link](https://jitpack.io/#sahabpardaz/regex-matcher). The jar file contains the C++ library 
in its resources and is ready to use. But you may want to know the build process as a contributor or 
if you want to build the jar file from scratch. If this is the case, this section is for you.

Due to the nature of the library, this library consists of two parts, Java and C++ source files.
The connection between the two parts is established through the JNI standard provided by Java.
For this reason, the compilation and build process of each of these parts is different.

### Building the C++ JNI library

If for any reason there is a change in the C++ source code side of the regex matcher, the JNI
library must be rebuilt. The C++ part of this project, after being built, is placed in the form of
a linux library file in the maven resource folder and is used by the Java program during execution.

Since compiling the C++ codes requires the `Chimera` library to be installed, all the requirements
for compiling the C++ source files are provided in the form of a docker file. This docker file,
in addition to providing all the requirements related to the C++ codes in an isolated environment,
also builds them automatically and puts the generated library file (.so file) in the required path
for the Java language. For this reason, the only requirement to rebuild the module is to have Docker installed.

First, apply the changes you want to make to the C++ source codes. Then execute below command:

```
src/main/cpp/build_lib_jni.sh
```

If changes are made correctly, the library will be built successfully by Docker and the new
library file will be copied to the appropriate path. You can further check it by running Java
tests in next step.

### Building the Java wrapper library

In both cases, where there is a change in source files of the Java wrapper or a change in the C++ library
you should build and test the Java library. Note that even if the changes in C++ library does not affect
the corresponding Java interfaces, you should build the Java library because the output jar file contains
the whole C++ library in its resources. To build the Java library and run its tests, just call Maven:

```
mvn clean package
```

After confirming the changes, you can publish the changes as a new version.

## Sample usage

Suppose you have a set of regular expression (regex) patterns through which you want to examine
a variety of string inputs. At first, create a new matcher instance and add the patterns you
want to match. By a boolean flag you will specify whether the pattern is case sensitive or not:

```java
RegexMatcher regexMatcher = new RegexMatcher();
regexMatcher.addPattern(1L, ".*w.*", true); // anything contains exactly 'w'
regexMatcher.addPattern(1L, ".*y.*", true); // anything contains exactly 'y'
regexMatcher.addPattern(2L, ".*z.*", false); // anything contains 'z' or 'Z'
```

It is legal to have multiple patterns, sharing the same ID:

```java
regexMatcher.addPattern(3L, ".*t.*", true); // anything contains exactly 't'
regexMatcher.addPattern(3L, ".*T.*", true); // anything contains exactly 'T'
```

Note that the patterns are dynamic, you can add or remove the patterns over time:

```java
regexMatcher.removePattern(3L);
```

In next step, we want to match the input strings against the patterns. But to make the
patterns effective, it is _necessary_ to first call the prepare method:

```java
regexMatcher.preparePatterns();

assertEquals(new HashSet<>(Arrays.asList(1L)), regexMatcher.match("text-contains-w-letter"));
assertEquals(new HashSet<>(Arrays.asList(1L)), regexMatcher.match("text-contains-y-letter"));
assertEquals(new HashSet<>(Arrays.asList(2L)), regexMatcher.match("text-contains-z-letter"));
assertEquals(new HashSet<>(Arrays.asList(1L, 2L)), regexMatcher.match("text-contains-wz-letter"));
assertEquals(new HashSet<>(Arrays.asList(1L, 2L)), regexMatcher.match("text-contains-wyz-letter"));
assertTrue(regexMatcher.match("t").isEmpty());
```

At the end, do not forget to release the resources by closing the matcher after the job is finished:

```java
regexMatcher.close()
```