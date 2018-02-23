(* @ML_PACKAGE_NAME@.m *)
BeginPackage["@ML_PACKAGE_NAME@`"]

Begin["`Private`"]

(* Find the binary location *)
$baseProgramName = "@ML_OUTPUT_NAME@";
$programName = $baseProgramName <> If[StringContainsQ[$SystemID, "Windows"], ".exe", ""];
$executable = FileNameJoin[{
	DirectoryName[$InputFileName],
	$SystemID,
	$programName
}];

(* Terminate existing connection *)
If[Head[$currentLink] == LinkObject, Uninstall[$currentLink]];

(* Create new connection *)
$currentLink = Install[$executable];

End[] (* `Private` *)

EndPackage[]
