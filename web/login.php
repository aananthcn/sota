<?php
echo ":-)";

if (isset($_POST['login'])) {
	$username=$_POST['login'];
}

if (isset($_POST['passw'])) {
	$password=$_POST['passw'];
}

$database="sotadb";
$tablehdr = array(
	"id" => "ID",
	"vin" => "VIN No",
	"name" => "Owner's Name",
	"phone" => "Phone No",
	"email" => "e-mail",
	"make" => "Make",
	"model" => "Model",
	"variant" => "Variant",
	"year" => "Year",
	"allowed" => "Update Flag",
	"state" => "Vehicle State",
	"dcount" => "Download Count",
	"ucount" => "Reflash Count",
	"udate" => "Last Update Date",
	"lcount" => "Login Count",
	"ldate" => "Last Login Date",
);

session_start();
$_SESSION['username'] = $username;
$_SESSION['password'] = $password;
$_SESSION['database'] = $database;
$_SESSION['tablehdr'] = $tablehdr;

header("Location: main.php");

?>
