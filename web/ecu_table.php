<html>
<title>Visteon SOTA</title>

<!--
<script src="http://code.jquery.com/jquery-latest.js"></script>

<script>
$(document).ready(function(){
	setInterval(function() {
		$("#ecu_tbl").load("refresh_main.php");
	}, 1000);
});
</script>
-->

<body>

<div id="fixed">
<font size='7px' color=#ff5f00><center>Visteon SOTA<br></center></font>
<p align="center"> <a href="main.php">Home Page</a></p>
<p align="right"> <a href="releases.php">Software Releases</a></p>

<form action="update_versions.php" method="post">
 <font face=arial size=3 color=#007f5f>
  ECU Name: <input type="text" name="ecu_name"/>
  New Version: <input type="text" name="new_version"/>
  <input type="submit" value="update"/>
 </font>
</form>
</div>

<div id="ecu_tbl">
<?php
/*************************************************************************
 * PHP CODE STARTS HERE
 */

function print_sota_table() {
	session_start();
	$username=$_SESSION['username'];
	$password=$_SESSION['password'];
	$database=$_SESSION['database'];

	mysql_connect(localhost,$username,$password);
	@mysql_select_db($database) or die( "Unable to select database");

	$vin = isset($_GET['content']) ? $_GET['content'] : '';
	$tbl = "ecus_"."$vin"."_tbl";
	$_SESSION['ecu_table']=$tbl;

	$query="select * from sotadb.$tbl";
	$ecu_tbl=mysql_query($query);

	$veh_rows=mysql_numrows($ecu_tbl);
	$veh_cols=mysql_num_fields($ecu_tbl);

	/* print table title */
	echo "<br><font size='4px' color=#ef4f00>";
	echo "<b>Vehicle VIN: $vin</b><br>";
	echo "</font>";
	echo "<br>No of ECUs: $veh_rows<br><br>";

	/* print vehicles - HEADER */
	echo "<table border=1><tr>";
	$i=0;while ($i < $veh_cols) {
		$meta = mysql_fetch_field($ecu_tbl, $i);
		echo "<th>$meta->name</th>";
		$i++;
	}
	echo "</tr>";

	/* print vehicles - DATA */
	$i=0;while ($i < $veh_rows) {
		$row=mysql_fetch_row($ecu_tbl);
		echo "<tr>";
		if($i & 1)
			$bgc = "#ffffff";
		else
			$bgc = "#dfefff";

		$j=0;while ($j < $veh_cols) {
			echo "<td bgcolor=$bgc ><font face=tahoma size=3>$row[$j]</font></td>";
			$j++;
		}
		echo "</tr>";
		$i++;
	}
	echo "</table>";

	mysql_free_result($ecu_tbl);
	mysql_close();
}


print_sota_table(); 

?>
</div>

</body>
</html>
