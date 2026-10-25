package app.sombra

import app.sombra.core.Calc
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class CalcTest {
    private fun ev(s: String) = Calc.eval(s)?.let(Calc::format)

    @Test fun basicOps() {
        assertEquals("7", ev("1+2×3"))
        assertEquals("9", ev("(1+2)×3"))
        assertEquals("2.5", ev("5÷2"))
        assertEquals("-3", ev("-5+2"))
        assertEquals("0.5", ev("50%"))
        assertEquals("0.3", ev("0.1+0.2"))
    }

    @Test fun invalid() {
        assertNull(ev("5÷0"))
        assertNull(ev(""))
        assertNull(ev("(1+2"))
    }

    @Test fun trailingOperatorIgnored() = assertEquals("12", ev("12+"))

    @Test fun pinMatchesAnyExpressionWithThatResult() {
        assertTrue(Calc.matchesPin(Calc.eval("1234")!!, "1234"))
        assertTrue(Calc.matchesPin(Calc.eval("1000+234")!!, "1234"))
        assertTrue(Calc.matchesPin(Calc.eval("2468÷2")!!, "1234"))
        assertFalse(Calc.matchesPin(Calc.eval("1234.5")!!, "1234"))
        assertFalse(Calc.matchesPin(Calc.eval("-1234")!!, "1234"))
        assertFalse(Calc.matchesPin(Calc.eval("1233")!!, "1234"))
    }
}
