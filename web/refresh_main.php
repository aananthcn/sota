<html>

<body>

<div id="sotatbl">

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


	$query="select * from sotadb.sotatbl ORDER BY id DESC";
	$sotatbl=mysql_query($query);

	$veh_rows=mysql_numrows($sotatbl);
	$veh_cols=mysql_num_fields($sotatbl);


	/* print vehicles - HEADER */
	echo "<table border=1><tr>";
	$i=0;while ($i < $veh_cols) {
		$meta = mysql_fetch_field($sotatbl, $i);
		echo "<th>$meta->name</th>";
		$i++;
	}
	echo "</tr>";

	/* print vehicles - DATA */
	$i=0;while ($i < $veh_rows) {
		$row=mysql_fetch_row($sotatbl);
		echo "<tr>";
		if($i & 1)
			$bgc = "#ffffff";
		else
			$bgc = "#dfefff";

		$j=0;while ($j < $veh_cols) {
			echo "<td bgcolor=$bgc ><font face=tahoma size=2>$row[$j]</font></td>";
			$j++;
		}
		echo "</tr>";
		$i++;
	}
	echo "</table>";

	mysql_free_result($sotatbl);
	mysql_close();
}

print_sota_table();
echo "<font face=arial size=2 color=#206080>";
echo date('l jS \of F Y h:i:s A');
echo "</font>";
?>

</div>
</body>
</html>
