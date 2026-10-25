package app.sombra.core

import java.math.BigDecimal
import java.math.MathContext
import java.math.RoundingMode

/**
 * Avaliador de expressões da calculadora de fachada.
 * Gramática: expr = termo (('+'|'-') termo)* ; termo = fator (('×'|'÷') fator)* ;
 * fator = ['-'] primário ['%']* ; primário = número | '(' expr ')'.
 * Retorna null para expressão inválida ou divisão por zero.
 */
object Calc {
    fun eval(raw: String): BigDecimal? {
        val s = raw.trim().trimEnd('+', '-', '×', '÷', '.')
        if (s.isEmpty()) return null
        return try {
            val p = Parser(s)
            val v = p.expr()
            if (p.pos != s.length) null else v
        } catch (_: ArithmeticException) {
            null
        } catch (_: IllegalArgumentException) {
            null
        }
    }

    /** Formata para exibição: até 10 casas, sem zeros à direita. */
    fun format(v: BigDecimal): String {
        val r = v.setScale(10, RoundingMode.HALF_UP).stripTrailingZeros()
        val txt = r.toPlainString()
        return if (txt == "-0") "0" else txt
    }

    /** O resultado só abre o cofre se for um inteiro não negativo igual à senha. */
    fun matchesPin(result: BigDecimal, pin: String): Boolean {
        val r = result.stripTrailingZeros()
        if (r.signum() < 0 || r.scale() > 0) return false
        return r.toBigInteger().toString() == pin
    }

    private class Parser(val s: String) {
        var pos = 0
        private val mc = MathContext.DECIMAL64

        fun expr(): BigDecimal {
            var v = term()
            while (pos < s.length && (s[pos] == '+' || s[pos] == '-')) {
                val op = s[pos++]
                val r = term()
                v = if (op == '+') v.add(r, mc) else v.subtract(r, mc)
            }
            return v
        }

        private fun term(): BigDecimal {
            var v = factor()
            while (pos < s.length && (s[pos] == '×' || s[pos] == '÷')) {
                val op = s[pos++]
                val r = factor()
                v = if (op == '×') v.multiply(r, mc) else v.divide(r, mc)
            }
            return v
        }

        private fun factor(): BigDecimal {
            var neg = false
            while (pos < s.length && s[pos] == '-') { neg = !neg; pos++ }
            var v = primary()
            while (pos < s.length && s[pos] == '%') {
                pos++
                v = v.divide(BigDecimal(100), mc)
            }
            return if (neg) v.negate() else v
        }

        private fun primary(): BigDecimal {
            if (pos < s.length && s[pos] == '(') {
                pos++
                val v = expr()
                require(pos < s.length && s[pos] == ')')
                pos++
                return v
            }
            val start = pos
            while (pos < s.length && (s[pos].isDigit() || s[pos] == '.')) pos++
            require(pos > start)
            return BigDecimal(s.substring(start, pos))
        }
    }
}
