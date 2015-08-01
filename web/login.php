<?php
echo ":-)";

if (isset($_POST['login'])) {
	$username=$_POST['login'];
}

if (isset($_POST['passw'])) {
	$password=$_POST['passw'];
}

$database="sotadb";

session_start();
$_SESSION['username'] = $username;
$_SESSION['password'] = $password;
$_SESSION['database'] = $database;

header("Location: main.php");

?>
