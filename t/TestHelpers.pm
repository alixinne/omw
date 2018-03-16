package TestHelpers;

use strict;
use warnings;
use Test::More;
use File::Spec;
use IPC::Open3;
use Getopt::Long;
require Exporter;

our @ISA = qw(Exporter);
our @EXPORT = qw(octave_ok mathematica_ok);

our $_initialized = 0;
our $_octave_pkg_name;
our $_mathematica_pkg_name;
our $_binary_path;

sub check_init {
	unless ($_initialized) {
		GetOptions("path=s" => \$_binary_path,
			"octave-pkg=s" => \$_octave_pkg_name,
			"mathematica-pkg=s" => \$_mathematica_pkg_name);

		$_initialized = 1;
	}
}

sub binary_path {
	return $_binary_path; # if undef, use the installed version by default
}

sub command_run {
	my ($cmd, $code, $message) = @_;
	# Print the code to be executed
	SKIP: {
		eval {
			# Start process
			my $pid = open3(\*CHLD_IN, \*CHLD_OUT, \*CHLD_OUT,
				$cmd);

			# Send code
			print CHLD_IN $code;

			# Get output
			my $output = do { local $/; <CHLD_OUT> };

			# Wait for exit
			waitpid($pid, 0);
			my $exit_status = $? >> 8;

			# Print code details
			diag "Running the following code in $cmd: ";
			$code =~ s/^/  /gm;
			diag $code;
			diag "Command output: ";
			$output =~ s/^/  /gm;
			diag $output;

			# Print output
			is($exit_status, 0, $message);
		};

		skip("$message (skipped because $@)", 1) if $@;
	}
}

sub octave_ok {
	my ($message, $code) = @_;
	$message = "Octave: $message";

	# Add code to load the library
	$code = <<OCTAVE_CODE;
$_octave_pkg_name();
$code
OCTAVE_CODE

	# Append code to load the current testing version
	if (defined binary_path) {
		my $path = File::Spec->catfile(binary_path, "$_octave_pkg_name.oct");
		$code = <<OCTAVE_CODE;
autoload("$_octave_pkg_name", "$path");
$code
OCTAVE_CODE
	}

	command_run 'octave-cli', $code, $message;
}

sub mathematica_ok {
	my ($message, $code) = @_;
	$message = "Mathematica: $message";

	# Append exit wrapper
	$code = <<MATHEMATICA_CODE;
On[Assert];
Exit[If[FailureQ[Check[Module[{},
<<$_mathematica_pkg_name`;
$code],\$Failed]],1,0]]
MATHEMATICA_CODE

	# Append code to load the current testing version
	if (defined binary_path) {
		my $path = binary_path;
		$code = <<MATHEMATICA_CODE;
PrependTo[\$Path, "$path"];
$code
MATHEMATICA_CODE
	}

	# Reformat code
	$code =~ s/;(?!\n)/;\n/g;

	command_run 'math', $code, $message;
}

check_init;

1;
